import numpy as np
import random

def sim_gen(data_in, BPE, plio_width, filename):
  data_per_col = (plio_width // 8) // BPE
  if data_in.ndim != 1:
    assert("Must transpose to one dim array before generating files")
  size = data_in.size
  with open(filename, 'w') as file:
    for i in range(size):
      data = data_in[i]
      file.write(f"{data}")
      if((i+1)%data_per_col==0):
        file.write("\n")
      else:
        file.write(" ")


def readFile(filename):
  rows = []
  # Open and read the file line by line
  with open(filename, 'r') as file:
    for line in file:
      if line.strip() and not any(char.isalpha() for char in line):
        row = list(map(float, line.split()))
        rows.append(row)
  array = np.array(rows)
  return array

def verification(file_sw, file_hw):
  array_sw = readFile(file_sw)
  array_hw = readFile(file_hw)
  flag = np.allclose(array_sw, array_hw)
  if flag:
    print("Test Passed\n")
  else:
    print("Test Failed\n")

np.random.seed(0)
data_0 = np.random.randint(0, 3, size=(32, 32))
data_1 = np.random.randint(0, 3, size=(32, 32))

data_0_1d = data_0.reshape(-1)
data_1_1d = data_1.reshape(-1)

# matmul
data_result = np.matmul(data_0, data_1)
data_result_1d = data_result.reshape(-1)

file_lhs = 'data/v0.txt'
file_rhs = 'data/v1.txt'
file_out = 'aiesimulator_output/data/v2.txt'
file_final = 'data/v2.txt'
file_golden = 'data/golden.txt'

sim_gen(data_0_1d, 4, 32, file_lhs)
sim_gen(data_1_1d, 4, 32, file_rhs)
sim_gen(data_result_1d, 4, 32, file_golden)

verify = True
if verify:
  array_hw = readFile(file_out)
  array_hw_1d = array_hw.reshape(-1)
  sim_gen(array_hw_1d, 4, 32, file_final)
  verification(file_golden, file_final)
   