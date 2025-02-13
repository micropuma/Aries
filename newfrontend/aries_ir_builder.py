import ast
import astor
import inspect
import sympy as sp
from .aries_decorators import TaskTileWrapper, TaskKernelWrapper, TaskTopWrapper

# =========== Code Transformation and IR Builder ===========
# =========== Parser and Emitter ===========
class MLIRGenerator(ast.NodeVisitor):
    def __init__(self, dmaInfo, map_cnt=0):
        self.mlir_func_code = [] # Store the code within func
        self.mlir_map_code = [] # Store the AffineMap code
        self.indent = 2
        self.var_count = 0
        self.map_cnt = map_cnt
        
        self.valName = {} # valName["A"] = "%A"
        self.valType = {} # E.g., valType["A"] = (T, [32, 32])
        self.valNameCnt = {} # valNameCnt["%A"] = cnt, to avoid name conflict
        self.map = {} # map["ti"] = # map, dims  now only record the offset map
        self.dmaInfo = dmaInfo # dic["ti"] = (32*i, 32, 1) (offset, size, step)
        self.in_assign  = False # Track whether inside an Assign node
        
    def add_var_name(self, val, name=""):
        if(name != ""):
            name = "%" + name
            if(self.valNameCnt.get(name, 0) > 0):
                valNameStr = name + str(self.valNameCnt[name])
                self.valNameCnt[name] += 1
            else:
                self.valNameCnt[name] = 1
                valNameStr = name
        else:
            valNameStr = "%v" + str(self.var_count)
            self.var_count += 1
        self.valName[val] = valNameStr
        return valNameStr
    
    def get_var_name(self, val):
        if val not in self.valName:
            assert False, f"KeyError: The val '{val}' was not defined."
        valNameStr = self.valName[val]
        return valNameStr
    
    def add_type_name(self, val, type, shape, memSpace = ""):
        self.valType[val] = (type, shape, memSpace)
        typeName = self.get_type_name(val)
        return typeName
        
    def get_eletype_name(self, ty):
        if(ty == "float32"):
          return "f32"
        elif(ty == "int32"):
          return "i32"
        elif(ty == "int16"):
          return "i16"
        elif(ty == "int8"):
          return "i8"
        elif(ty == "index"):
          return "index"
        else:
          raise TypeError(f"Type {ty} not supported.")
    
    def addMemSpace(self, type_name, memSpace):
        if memSpace == "L1":
            type_name += ", " + str(2)
        elif memSpace == "L2":
            type_name += ", " + str(1)
        elif memSpace == "L3":
            type_name += ", " + str(0)
        return type_name
    
    def get_MemRefType_name(self, ty, shape, memSpace):
      shape_name = "x".join([str(s) for s in shape])
      type_name = self.get_eletype_name(ty)
      type_name = self.addMemSpace(type_name, memSpace)
      memrefType_name = f"memref<{shape_name}x{type_name}>"
      return memrefType_name
    
    def get_type_name(self, arg):
        if arg not in self.valType:
            assert False, f"KeyError: The type of '{arg}' was not found."
        ty, shape, memSpace = self.valType[arg]
        if len(shape) == 0:
            typeName = self.get_eletype_name(arg, ty)
        else:
            typeName = self.get_MemRefType_name(ty, shape, memSpace)
        return typeName
    
    def is_declared(self, arg):
        if arg in self.valName:
            return True
        else:
            return False
    
    def is_array(self, arg):
        if arg not in self.valType:
            assert False, f"KeyError: The type of '{arg}' was not found." 
        _, shape, _ = self.valType[arg] 
        if len(shape) == 0:
            return False
        else:
            return True       
    
    def addArgType(self, node):
        # Record the type of arguments in the function
        for arg in node.args.args:
            self.add_var_name(arg.arg, arg.arg)
            if arg.annotation:
                ty = arg.annotation
                if isinstance(ty, ast.Subscript):
                    # Extract shape
                    shape = tuple(constant.value for constant in ty.slice.elts)
                    self.add_type_name(arg.arg, ty.value.id, shape)
                else:
                    assert False, "Unspported argument type found!"
        return
    
    def visit_FunctionDef(self, node):
        # Add types for each arg
        self.addArgType(node)
        
        # Process the function definition and arguments
        func_name = node.name
        args = [arg.arg for arg in node.args.args]
        # Get annotation types
        memrefs = []
        for arg in args:
            if arg in self.valType:
                typeName = self.get_type_name(arg)
                memrefs.append(f"%{arg}: {typeName}")
            else:
                raise ValueError(f"Argument {arg} not found in valType.")
        self.emit(f"func.func @{func_name}({', '.join(memrefs)}) {{")

        self.indent += 2
        self.generic_visit(node)
        self.emit("return")
        self.indent -= 2
        self.emit("}")
    
    def visit_For(self, node):
        # Process affine.for loop
        loop_var = node.target.id  # e.g., i0, j0, k0
        loop_Name = self.add_var_name(loop_var, loop_var)
        loop_call = node.iter.func.id
        loop_lb = node.iter.args[0].value 
        loop_ub = node.iter.args[1].value 
        self.emit(f"affine.for {loop_Name} = {loop_lb} to {loop_ub} {{")

        self.indent += 2
        self.generic_visit(node)
        self.indent -= 2

        if loop_call == "reduction_range":
          self.emit("}{reduction}")
        else:
          self.emit("}")
    
    def getDmaInfo(self, call):
        # Stores the VarName of offsets, sizes and strides
        # that can be printed out directly
        offsets = []
        sizes = []
        strides = []
        srcMemName = None
        dstMemName = None
        name_cnt = 0
        for arg in call.args:
            if isinstance(arg, ast.Name):
                if name_cnt == 0:
                    srcMemName = arg.id
                    name_cnt += 1
                else:
                    dstMemName = arg.id
            elif isinstance(arg, ast.Tuple):
                for elt in arg.elts:
                    eltName = elt.id
                    # For each dim, create the affine.apply op for each offset
                    mapName, dims = self.map[eltName] #mapName, dims
                    dimNames = [self.get_var_name(dim) for dim in dims]
                    offsetVar = elt.id + "_offset"
                    offsetVarName = self.add_var_name(offsetVar)
                    offsets.append(offsetVarName)
                    self.emit(f"{offsetVarName} = affine.apply {mapName}({', '.join(dimNames)})")
                    # For each dim, create constant for size and stride
                    _, size, step = self.dmaInfo[eltName]
                    sizeVar = elt.id + "_size"
                    sizeVarName = self.add_var_name(sizeVar, "c" + sizeVar)
                    sizes.append(sizeVarName)
                    self.emit(f"{sizeVarName} = arith.constant {size} : index")
                    strideVar = elt.id + "_stride"
                    strideVarName = self.add_var_name(strideVar, "c" + strideVar)
                    strides.append(strideVarName)
                    self.emit(f"{strideVarName} = arith.constant {step} : index")
        return offsets, sizes, strides, srcMemName, dstMemName
    
    def visit_Assign(self, node):
        assert len(node.targets) == 1
        target = node.targets[0]
        
        if isinstance(node.value, ast.Call):
            assert isinstance(node.value.func, ast.Attribute)
            assert isinstance(node.value.func.value, ast.Name)
            assert node.value.func.value.id == 'aries'
            
            if node.value.func.attr == 'load':
                call = node.value
                dstMemName = target.id
                offsets, sizes, strides, srcMemName, _ = self.getDmaInfo(call)
                srcMem = self.get_var_name(srcMemName)
                srcType = self.get_type_name(srcMemName)
                dstMem = self.get_var_name(dstMemName)
                dstType = self.get_type_name(dstMemName)
                self.emit(f"adf.dma({srcMem}[{' ,'.join(offsets)}] [{' ,'.join(sizes)}] [{' ,'.join(strides)}], {dstMem}[] [] []) : ({srcType} , {dstType})")  
                return
            
            elif node.value.func.attr == 'buffer':
                buffer = target.id
                self.add_var_name(buffer, buffer)
                shape_node = node.value.args[0]
                type_node = node.value.args[1]
                shape = tuple(constant.value for constant in shape_node.elts)
                type = type_node.value
                typeName = self.add_type_name(buffer, type, shape, "L1")
                bufferName = self.get_var_name(buffer)
                self.emit(f"{bufferName} = adf.buffer.create @L1_{buffer}() : {typeName}")
                return

        # For Task kernel
        elif isinstance(node.value, ast.BinOp):
            self.in_assign = True 
        # if isinstance(target, ast.Subscript):
        #     if isinstance(node.value, ast.BinOp):
        #         self.generate_binop(node)
        #     array_name = self.get_var_name(target.value.id)
        #     ivNames = [self.get_var_name(elt.id) for elt in target.slice.elts]
            
            
            
    # def visit_Name(self, node):
    #     """Visit a variable and return its MLIR representation."""
    #     return self.get_var_name(node.id)
    
    # def visit_Constant(self, node):
    #     """Visit a constant and return MLIR representation."""
    #     value = node.value
    #     result_var = f"%c{len(self.mlir_code)}"
    #     self.mlir_code.append(f"{result_var} = arith.constant {value} : f32")
    #     return result_var
    
    # def visit_BinOp(self, node):
    #     """Visit a binary operation node."""
    #     return self.generate_binop(node)
    
    def generate_binop(self, node):
        """Recursively parse BinOp and generate MLIR code."""
        lhs = self.visit(node.left)
        rhs = self.visit(node.right)
        
        return
    
    def visit_Expr(self, node):
        if isinstance(node.value, ast.Call):
            if isinstance(node.value.func, ast.Name):
                calleeName = node.value.func.id
                args = node.value.args
                argNames = []
                argTypes = []
                for arg in args:
                    argNames.append(self.get_var_name(arg.id))
                    argTypes.append(self.get_type_name(arg.id))
                self.emit(f"func.call @{calleeName}({', '.join(argNames)}) : (")
                self.emit(f"{', '.join(argTypes)}) -> ()", True)
            elif isinstance(node.value.func, ast.Attribute):
                if node.value.func.attr == 'store':
                    call = node.value
                    offsets, sizes, strides, srcMemName, dstMemName = self.getDmaInfo(call)
                    srcMem = self.get_var_name(srcMemName)
                    srcType = self.get_type_name(srcMemName)
                    dstMem = self.get_var_name(dstMemName)
                    dstType = self.get_type_name(dstMemName)
                    self.emit(f"adf.dma({srcMem}[] [] [], {dstMem}[{' ,'.join(offsets)}] [{' ,'.join(sizes)}] [{' ,'.join(strides)}]) : ({srcType} , {dstType})")  
                    return
    
    def emit(self, code, same_line=False):
        if same_line:
            self.mlir_func_code[-1] = self.mlir_func_code[-1] + code
        else:
            self.mlir_func_code.append(" " * self.indent + code)
    
    def emitMap(self, code):
        self.mlir_map_code.append(code)
    
    def find_dim(self, expr):
        """
        Extract dimension variables from a SymPy expression.

        Args:
            expr: SymPy expression (e.g., i * 32 + j)

        Returns:
            List of dimension variable names (sorted for consistency)
        """
        return sorted([str(symbol) for symbol in expr.free_symbols])  # Extract symbols as strings


    def emit_affine_map(self, expr, dims, syms):
        """
        Convert a SymPy expression to an MLIR affine map.

        Args:
            expr: SymPy expression (e.g., i * 32 + j)
            dims: List of dimension variables (e.g., ['i', 'j'])
            syms: List of symbolic variables (e.g., [])

        Returns:
            MLIR affine map string (e.g., affine_map<(d0, d1) -> (d0*32 + d1)>)
        """
        dim_map = {dim: f"d{i}" for i, dim in enumerate(dims)}
        sym_map = {sym: f"s{i}" for i, sym in enumerate(syms)}

        affine_expr = expr.subs(dim_map).subs(sym_map)

        dim_str = ", ".join(dim_map.values()) if dim_map else ""
        sym_str = ", ".join(sym_map.values()) if sym_map else ""

        map_str = f"affine_map<({dim_str})"
        if sym_str:
            map_str += f"[{sym_str}]"
        map_str += f" -> ({affine_expr})>"

        return map_str
    
    # Generate map for offset
    def gen_map(self):
        if self.dmaInfo is None:
            return
        for key, value in self.dmaInfo.items():
            offestExpr = value[0]
            dims = self.find_dim(offestExpr)
            affineMap = self.emit_affine_map(offestExpr, dims, [])
            mapName = "#map" + str(self.map_cnt)
            self.map_cnt +=1
            self.map[key] = (mapName, dims)
            self.emitMap(f"{mapName} = {affineMap}")
        return
    
    def generate(self, tree):
        # Generate the code for AffineMap
        self.gen_map()
        
        # Parse the input Python code and visit nodes
        self.visit(tree)
        func_code = "\n".join(self.mlir_func_code)
        map_code = "\n".join(self.mlir_map_code)
        return func_code, map_code, self.map_cnt

class TileToLoop(ast.NodeTransformer):
    """Transform tile ranks to loops"""
    def __init__(self, grid_dims=None):
        # tile sizes and grid ranks
        self.grid_dims = grid_dims
    
    def visit_FunctionDef(self, node):
        # Extract assignment to i, j
        new_body = []
        loop_vars = {}

        for stmt in node.body:
            if (isinstance(stmt, ast.Assign) and 
                stmt.value.func.value.id == "aries" and 
                stmt.value.func.attr == "tile_ranks"):
                loop_var_names = [elt.id for elt in stmt.targets[0].elts]
                for idx, var in enumerate(loop_var_names):
                    loop_vars[var] = self.grid_dims[idx]
                continue
            
            new_body.append(stmt)

        for var, max_range in reversed(loop_vars.items()):
            new_body = [ast.For(
                target=ast.Name(id=var, ctx=ast.Store()),
                iter=ast.Call(
                    func=ast.Name(id="range", ctx=ast.Load()),
                    args=[ast.Constant(value=0), ast.Constant(value=max_range)],
                    keywords=[]
                ),
                body=new_body,
                orelse=[]
            )]

        node.body = new_body  # Replace function body with transformed version
        return node

# Traverse the tree for constant propagation and record arange info
class TilePreprocess(ast.NodeTransformer):
    def __init__(self, grid_dims=None, tile_sizes=None):

        # tile sizes and grid ranks
        self.grid_dims = grid_dims
        self.tile_sizes = tile_sizes

        # Record the constant value
        self.constant_mapping = {}
        
        # Record the offset, size, step information from aries.arange
        self.dmaInfo = {} # dic["ti"] = (32*i, 32, 1) (offset, size, step)
        self.startExpr = None
        self.stopExpr = None
        self.stepExpr = sp.Integer(1) # Default step is 1 if not assigned

    def visit_FunctionDef(self, node):
        # Traverse function body and update
        node.body = [self.visit(statement) for statement in node.body]
        return node
    
    def generic_visit(self, node):
        return super().generic_visit(node)
    
    # Recursively converts a nested BinOp AST node into a SymPy expression
    def arange_to_sympy(self, node):
        if isinstance(node, ast.BinOp):
            left = self.arange_to_sympy(node.left)  # Recursively process left operand
            right = self.arange_to_sympy(node.right)  # Recursively process right operand

            if isinstance(node.op, ast.Add):
                return left + right
            elif isinstance(node.op, ast.Sub):
                return left - right
            elif isinstance(node.op, ast.Mult):
                return left * right
            elif isinstance(node.op, ast.Div):
                return left / right
            elif isinstance(node.op, ast.Mod):
                return left % right
            elif isinstance(node.op, ast.Pow):
                return left**right
            else:
                raise ValueError(f"Unsupported operation: {node.op}")

        elif isinstance(node, ast.Name):
            return sp.Symbol(node.id)  # Convert variable names to SymPy symbols
        elif isinstance(node, ast.Constant):
            return sp.Integer(node.value)  # Convert constants to SymPy Integers

        raise ValueError(f"Unsupported node type: {type(node)}")
    
    def visit_Assign(self, node):
        assert len(node.targets) == 1
        target = node.targets[0]
        
        if isinstance(node.value, ast.Call):
            assert isinstance(node.value.func, ast.Attribute)
            assert isinstance(node.value.func.value, ast.Name)
            assert node.value.func.value.id == 'aries'
            
            # TI, TJ, TK = tile_sizes()
            # Record the tile size which are constant values
            if node.value.func.attr == "tile_sizes":
                assert isinstance(target, ast.Tuple)
                assert len(target.elts) == len(self.tile_sizes)

                for i, tsize in enumerate(self.tile_sizes):
                    self.constant_mapping[target.elts[i].id] = int(tsize)
                return None
            
            # Need to visit the nested nodes to propagate constants and 
            # record the arange Info
            elif node.value.func.attr == "arange":
                call = node.value
                # Recursively visit and propagate constants
                call.args = [self.visit(arg) for arg in call.args]
                self.startExpr = None
                self.stopExpr = None
                self.stepExpr = sp.Integer(1)
                # Record the arange information
                for idx, arg in enumerate(call.args):
                    if idx == 0: # Processes start
                        self.startExpr = self.arange_to_sympy(arg)
                    elif idx == 1: # Processes stop
                        self.stopExpr = self.arange_to_sympy(arg)
                    elif idx == 1: # Processes step
                        self.stepExpr = self.arange_to_sympy(arg)
                # offset = start, size = (stop-start)/step
                sizeExpr = (self.stopExpr-self.startExpr) / self.stepExpr
                coeffs = sizeExpr.as_coefficients_dict()
                for coeff in coeffs.values():
                    assert coeff.is_integer, f"Non-integer coefficient found: {coeff}"
                sp.simplify(self.startExpr)
                sp.simplify(sizeExpr)
                sp.simplify(self.stepExpr)
                self.dmaInfo[target.id] = (self.startExpr, sizeExpr, self.stepExpr)
                return node
            
            elif node.value.func.attr == "buffer":
                call = node.value
                # Recursively visit and propagate constants
                call.args = [self.visit(arg) for arg in call.args]
                return node
            
            elif node.value.func.attr in ["load", "store"]:
                return node
            
            else:
                raise ValueError(
                    f"Unsupported function call: {node.value.func.attr}")
        return node
    
    def visit_Name(self, node):
        # Replace variable names in constant_mapping with their corresponding values
        if node.id in self.constant_mapping:
            return ast.Constant(value=self.constant_mapping[node.id], kind=None)
        return node

    def visit_BinOp(self, node):
        # Recursively replace variable names inside binary operations
        node.left = self.visit(node.left)
        node.right = self.visit(node.right)
        return node

# =========== Schedule ===========
class Schedule:
    def __init__(self, *tasks):
        self.tasks = tasks
        self.flags = ""
        self.mlir_func_code = []
        self.mlir_map_code = [] # AffineMap should be defined outside of Module
        self.map_cnt = 0
    
    # A helper function to collect funcs from the given module
    def collect_func(self, module):
        funcs= []
        top_func = []
        for _, obj in module.items():
            if isinstance(obj, TaskKernelWrapper):
                funcs.append(obj.func)
            elif isinstance(obj, TaskTileWrapper):
                funcs.append(obj.func)
            elif isinstance(obj, TaskTopWrapper):
                top_func.append(obj.func)
        return funcs, top_func
    
    def task_tile_emit(self, func_name, parsed_ast):
        task = None
        # TODOs: Now assumes tasks has unique funcs 
        for task_temp in self.tasks:
            if task_temp.func.__name__ == func_name:
                task = task_temp
                break
            
        # Transform the tile ranks to index
        tileTrans = TileToLoop(task.grid_dims)
        tree = tileTrans.visit(parsed_ast)
        ast.fix_missing_locations(tree)
        # print("Parsed New AST 0", ast.dump(tree, indent=4))
        # print("\n\n\n=== Python AST 0 code ===")
        # print(astor.to_source(tree))
        
        # Collect dmaInfo and conduct constant propagation
        preInstance = TilePreprocess(task.grid_dims, task.tile_sizes)
        tree = preInstance.visit(tree)
        ast.fix_missing_locations(tree)
        dmaInfo = preInstance.dmaInfo
        # print("Parsed New AST 1", ast.dump(tree, indent=4))
        
        # Add func and map code
        func_code, map_code, self.map_cnt = MLIRGenerator(dmaInfo, self.map_cnt).generate(tree)
        self.mlir_func_code.append(func_code)
        print(func_code)
        self.mlir_map_code.append(map_code)
    
    def task_kernel_emit(self, parsed_ast):
        print("Parsed AIE Kernel AST", ast.dump(parsed_ast, indent=4))
        func_code, map_code, self.map_cnt = MLIRGenerator(None, self.map_cnt).generate(parsed_ast)
        self.mlir_func_code.append(func_code)
        self.mlir_map_code.append(map_code)
        print(func_code)
        
    def build(self, module):
        funcs, top_func = self.collect_func(module)
        assert len(top_func) == 1
        
        for func in funcs:
            source_code = inspect.getsource(func)
            parsed_ast = ast.parse(source_code)
            decorator_id = None
            func_name = None
            # Identify the decorators and remove the decorator_list
            for node in ast.walk(parsed_ast):
                if isinstance(node, ast.FunctionDef):
                    # Assume there's only one decorator for each func
                    decorator_id = node.decorator_list[0].func.id
                    func_name = node.name
                    break
            if decorator_id is None:
                continue
            if decorator_id == 'task_kernel':
                self.task_kernel_emit(parsed_ast)
                continue
            elif decorator_id == 'task_tile':
                self.task_tile_emit(func_name, parsed_ast)
                continue
            elif decorator_id == 'task_top':
                continue
            else:
                raise ValueError(f"Unsupported decorator: {decorator_id}")