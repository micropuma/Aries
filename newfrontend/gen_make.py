from jinja2 import Environment, FileSystemLoader

def gen_make_aries(prj_dir, template_dir, func, paraSize, l2Size):
    environment = Environment(loader=FileSystemLoader(template_dir))
    file_name = 'Makefile_ARIES'
    template = environment.get_template(file_name)
    file_tmp = "Makefile"
    filename = prj_dir / file_tmp
    dim = len(paraSize)
    
    content = template.render(
      func = func,
      parallel = paraSize,
      l2buffer = l2Size,
      dim = dim,
      template_dir = template_dir
    )
    with open(filename, mode="w", encoding="utf-8") as message:
        message.write(content)
        print(f"... wrote {filename}")