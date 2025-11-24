import os
import re
import json
import numpy as np
import matplotlib.pyplot as plt


def parse_filename(filename):
    match = re.match(r"QPE16_(\d+)QPUs\.json", filename)
    if match:
        num_qpus = int(match.group(1))
        if num_qpus == 1:
            return None
        return num_qpus
    return None

def binary_to_decimal(bit_string):
    number = 0
    for i, bit in enumerate(bit_string[::-1]):
        number+= int(bit)*2**i
    return number

def calc_phase(counts):

    sorted_pairs = sorted(zip(counts.values(), list(counts.keys())))

    if len(sorted_pairs) == 1:
        return binary_to_decimal(sorted_pairs[0][1])/(2**16)
    
    max1,max2 = sorted_pairs[-1], sorted_pairs[-2]

    return (binary_to_decimal(max1[1])/(2**16), binary_to_decimal(max2[1])/(2**16))


    

directory = "./results_QPE/"

num_qpus_list1 = []
counts_list1 = []

num_qpus_list2 = []
counts_list2 = []

for filename in os.listdir(directory):
    print(filename)
    num_qpus = parse_filename(filename)
    if num_qpus is not None:
        filepath = os.path.join(directory, filename)
        with open(filepath, "r") as f:
            data = json.load(f)
            for k,v in enumerate(data.values()):
                counts = v.get("counts")
                if counts is not None:
                    if k == 0:
                        num_qpus_list1.append(num_qpus)
                        counts_list1.append(counts)
                    elif k == 1:
                        num_qpus_list2.append(num_qpus)
                        counts_list2.append(counts)


thetas1 = [calc_phase(count) for count in counts_list1]

thetas2 = [calc_phase(count) for count in counts_list2]

result = {"theta1":sorted(zip(num_qpus_list1, thetas1)), "theta2":sorted(zip(num_qpus_list2, thetas2))}

with open(f"./results_QPE/thetas.json", "w") as f:
    json.dump(result, f, indent=2)
