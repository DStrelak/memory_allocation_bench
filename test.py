#!/usr/bin/python3
import random
import subprocess
import pandas as pd
import numpy as np


benchmark = []
SEPARATOR = '|'


def progressBar(iterable, prefix = '', suffix = '', decimals = 1, length = 100, fill = '█', printEnd = "\r"):
    """
    Call in a loop to create terminal progress bar
    @params:
        iteration   - Required  : current iteration (Int)
        total       - Required  : total iterations (Int)
        prefix      - Optional  : prefix string (Str)
        suffix      - Optional  : suffix string (Str)
        decimals    - Optional  : positive number of decimals in percent complete (Int)
        length      - Optional  : character length of bar (Int)
        fill        - Optional  : bar fill character (Str)
        printEnd    - Optional  : end character (e.g. "\r", "\r\n") (Str)
    """
    total = len(iterable)
    # Progress Bar Printing Function
    def printProgressBar (iteration):
        percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
        filledLength = int(length * iteration // total)
        bar = fill * filledLength + '-' * (length - filledLength)
        print(f'\r{prefix} |{bar}| {percent}% {suffix}', end = printEnd)
    # Initial Call
    printProgressBar(0)
    # Update Progress Bar
    for i, item in enumerate(iterable):
        yield item
        printProgressBar(i + 1)
    # Print New Line on Complete
    print()


def to_bytes(mb):
    return mb * 1024 * 1024


def to_gigabytes(bytes):
    return bytes / float(1024 * 1024 * 1024)


def to_megabytes(bytes):
    return bytes / float(1024 * 1024)


def to_seconds(microseconds):
    return microseconds / float(1024 * 1024)


def run(mode, count, use_madvise):
    cmd = ["./benchmark", str(mode), str(count), use_madvise]
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    return process.communicate()


def get_filename():
    import socket
    return 'mem_bench_' + str(socket.gethostname()) + '.csv'


def get_filename_stat():
    import socket
    return 'mem_bench_stat_' + str(socket.gethostname()) + '.csv'


def parse(logs):
    if logs[1]:
        return ['error', logs[1].decode().strip(), 0, 0, '']
    # expected format:
    # mmap+loop_set(double)|madvise ON|4096|17|us
    # p: 0x7fec95afc000
    lines = logs[0].decode().strip().split('\n')
    tokens = [l.strip().split(SEPARATOR) for l in lines if 'p: 0x' not in l]
    return tokens


def store(df):
    name = get_filename()
    print('Results stored in ' + name)
    df.to_csv(name, sep=SEPARATOR)


def store_statistics(df):
    stat = df.groupby(['method', 'madvise flag'])['GB/s'].agg(['mean', 'median', 'std'])
    name = get_filename_stat()
    print('Results stored in ' + name)
    stat.to_csv(name, sep=SEPARATOR)


def as_df():
    names = ['method', 'madvise flag', 'bytes', 'duration', 'unit']
    df = pd.DataFrame(benchmark, columns=names)
    df['GB/s'] = to_gigabytes(df['bytes'].astype(float)) / to_seconds(df['duration'].astype(float))
    return df


def main():
    random.seed(42)
    for i in progressBar(range(100), prefix = 'Progress', suffix='Complete', length=50):
        mb = int(random.uniform(20, 4096))
        bytes = to_bytes(mb)
        for j in range(0, 40):
            benchmark.extend(parse(run(j, bytes, 'madvise')))
            benchmark.extend(parse(run(j, bytes, '')))
    
    df = as_df()
    store(df)
    store_statistics(df)


if __name__ == "__main__":
    main()
