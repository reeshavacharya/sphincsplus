#! /usr/bin/env python3
import fileinput
import itertools
import os
import sys
import re
import csv
from subprocess import DEVNULL, run, PIPE

implementations = [
                   ('ref', ['shake', 'sha2', 'haraka']),
                   ('haraka-aesni', ['haraka']),
                   ('shake-avx2', ['shake']),
                   ('sha2-avx2', ['sha2']),
                   ]

options = ["f", "s"]
sizes = [128, 192, 256]
thashes = ['robust', 'simple']

# Where to store summarized results (averages only).
OUT_CSV = os.path.join(os.path.dirname(__file__), 'benchmarks_summary.csv')


def parse_averages(output_text):
    """
    Parse the stdout of the 'make benchmark' run and extract average times (ns)
    for key generation, signing, and verification.

    Returns a tuple: (keygen_avg, sign_avg, verify_avg) as integers or None if missing.
    """
    keygen_avg = sign_avg = verify_avg = None
    current = None
    for raw in output_text.splitlines():
        line = raw.strip()
        if line.startswith('Key Generation'):
            current = 'keygen'
            continue
        if line.startswith('Signing'):
            current = 'sign'
            continue
        if line.startswith('Verification'):
            current = 'verify'
            continue
        if line.startswith('Average:'):
            m = re.search(r'Average:\s*([0-9]+)', line)
            if m:
                val = int(m.group(1))
                if current == 'keygen':
                    keygen_avg = val
                elif current == 'sign':
                    sign_avg = val
                elif current == 'verify':
                    verify_avg = val
    return keygen_avg, sign_avg, verify_avg

def main():
    # Prepare CSV writer (overwrite on each run for a clean summary)
    with open(OUT_CSV, mode='w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow([
            'implementation', 'function_family', 'security_level', 'variant', 'thash',
            'paramset', 'keygen_avg_ns', 'sign_avg_ns', 'verify_avg_ns'
        ])

        for impl, fns in implementations:
            # params_path = os.path.join(impl, "params.h")  # not used directly here
            for fn in fns:
                for opt, size, thash_val in itertools.product(options, sizes, thashes):
                    paramset = "sphincs-{}-{}{}".format(fn, size, opt)
                    paramfile = "params-{}.h".format(paramset)

                    print("Benchmarking", paramset, thash_val, "using", impl, flush=True)

                    params_arg = 'PARAMS={}'.format(paramset)  # overrides Makefile var
                    thash_arg = 'THASH={}'.format(thash_val)  # overrides Makefile var

                    # Clean and build the time benchmark (test_time)
                    run(["make", "-C", impl, "clean", thash_arg, params_arg, "EXTRA_CFLAGS=-w"],
                        stdout=DEVNULL, stderr=sys.stderr, check=False)
                    run(["make", "-C", impl, "test/test_time", thash_arg, params_arg, "EXTRA_CFLAGS=-w"],
                        stdout=DEVNULL, stderr=sys.stderr, check=True)

                    # Run the test_time binary and capture output for parsing
                    completed = run(["./test/test_time"], cwd=impl,
                                    stdout=PIPE, stderr=sys.stderr, text=True, check=True)

                    keygen_avg, sign_avg, verify_avg = parse_averages(completed.stdout)

                    if None in (keygen_avg, sign_avg, verify_avg):
                        print(f"Warning: Could not parse averages for {impl} {paramset} {thash_val}", file=sys.stderr)

                    writer.writerow([
                        impl, fn, size, opt, thash_val,
                        paramset, keygen_avg, sign_avg, verify_avg
                    ])

                    print(flush=True)


if __name__ == '__main__':
    main()

