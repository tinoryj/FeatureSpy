from os import replace
import random
from tqdm import tqdm
import getopt
import sys
import time
import math

alphabet_set = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f']
fp_size = 6
chunk_size = 8192

def Usage():
    print("{name} -s [total_chunk_number] -d [deduplication factor] -o [output file] -c [zpif constant]".format(name=__file__))

def ZipfNormalizedFreq(zipf_divider, k, s):
    freq = 1 / (k**s)
    freq = freq / zipf_divider
    return freq


def ComputeZipfDivider(n, s):
    zipf_divider = 0
    for i in range(1, n+1):
        tmp = 1 / (i**s)
        zipf_divider = zipf_divider + tmp
    return zipf_divider

def GenerateUniqueChunkList(unique_chunk_list: list, unique_file_num):
    for i in tqdm(range(unique_file_num)):
        chunk_fp = ""
        for item in range(fp_size):
            chunk_fp = chunk_fp + alphabet_set[random.randrange(0, len(alphabet_set))]
            chunk_fp = chunk_fp + alphabet_set[random.randrange(0, len(alphabet_set))]
            chunk_fp = chunk_fp + ":"
        chunk_fp = chunk_fp[:-1]
        unique_chunk_list.append(chunk_fp)
    print("Generate the unique data set done!")

def GenerateLogicalChunkList(output_file_name: str, unique_chunk_list: list, logical_chunk_num, freq_list: list):
    output_file = open(output_file_name, mode='w')
    for i in tqdm(range(logical_chunk_num)):
        if (i < len(unique_chunk_list)):
            output_file.write(unique_chunk_list[i] + "\t\t" + str(chunk_size) + "\t\t10" + "\n")
        else:
            output_file.write(random.choices(population=unique_chunk_list, weights=freq_list)[0] + "\t\t" + str(chunk_size) + "\t\t10" + "\n")
    output_file.close()
    print("Output the trace file done!")  

def GenerateLogicalChunkListNew(output_file_name: str, unique_chunk_list: list, logical_chunk_num, freq_list: list):
    output_file = open(output_file_name, mode='w')
    for i in range(len(unique_chunk_list)):
        output_file.write(unique_chunk_list[i] + "\t\t" + str(chunk_size) + "\t\t10" + "\n")
    it_time = logical_chunk_num / len(unique_chunk_list) - 1
    for i in tqdm(range(int(it_time))):
        chunk_fp_list = random.choices(population=unique_chunk_list, weights=freq_list, k=len(unique_chunk_list))
        for item in range(len(chunk_fp_list)):
            output_file.write(chunk_fp_list[item] + "\t\t" + str(chunk_size) + "\t\t10" + "\n")
    output_file.close()
    print("Output the trace file done!")
            
if __name__ == "__main__":
    uniqueFileSizeNumber = 1017
    targetSnapSize = 0
    output_file_name = ""
    a = 1
    opts, args = getopt.getopt(sys.argv[1:], "-s:-c:-d:-o:-h")
    for opt_name, opt_value in opts:
        if opt_name == '-s':
            targetSnapSize = int(opt_value)
        elif opt_name == '-c':
            a = float(opt_value)
            print(a)
        elif opt_name == '-h':
            Usage()
            exit()
        elif opt_name == '-o':
            output_file_name = opt_value
        else:
            Usage()
            exit()

    random.seed(time.time())
    zipf_divider = ComputeZipfDivider(uniqueFileSizeNumber, a)

    freq_list = []
    print("Generate the freq distribution")
    for i in tqdm(range(1, uniqueFileSizeNumber + 1)):
        tmp_freq = ZipfNormalizedFreq(zipf_divider, i, a);
        freq_list.append(tmp_freq)
    # print("sum: {sum_val}".format(sum_val=sum(freq_list)))
    # print("Low freq = ", freq_list[uniqueFileSizeNumber-1])
    # print(freq_list)
    totalSize = 0
    for i in range(8, 1024):
        totalSize = totalSize + freq_list[i-8] * i
    # print("Origin size (KiB) = ", totalSize)
    # print("Target size (MiB) = ", targetSnapSize)
    totalGenerateFileNumber = round(targetSnapSize * 1024 / totalSize)
    print("Total generate file number = ", totalGenerateFileNumber)
    temp = 0
    tempSize = 0
    output_file = open(output_file_name, mode='w')

    for i in range(0, uniqueFileSizeNumber):
        temp = temp + round(freq_list[i]*totalGenerateFileNumber)
        tempSize = tempSize + round(freq_list[i]*totalGenerateFileNumber) * (i + 8)
        # print(round(freq_list[i]*totalGenerateFileNumber))
        fileSize = i + 8
        output_file.write(str(round(freq_list[i]*totalGenerateFileNumber)) + "\n" + str(fileSize) + "\n")
        # if tempSize > targetSnapSize * 1024:
        #     break
    output_file.close()
    print("generate file number = ",temp)
    print("generate file total size = ",tempSize/1024/1024)