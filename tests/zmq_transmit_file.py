import zmq
from time import sleep
import numpy as np

def complex64_to_u1(signal):
    signal = signal.astype('complex64');
    signal = signal / np.max(np.abs(signal));
    signal = signal.astype('complex64').view('float32');
    signal = signal * 127.5;
    signal = np.round(signal + 127.5).astype('float32');
    signal = signal.view('complex64');
    c = np.empty((signal.size*2,), dtype='u1')
    c[0::2] = np.real(signal);
    c[1::2] = np.imag(signal);
    return c;

def u1_to_complex64(samples):
    samples = samples.astype('f4');
    samples = (samples - 127.5)/127.5;
    samples = samples.view('complex64');
    return samples;

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("iq-file", help="File to transmit data from");
parser.add_argument("--period", help="Transmission period", type=float, default=0);
parser.add_argument("--format", help="Transmission file format", type=str, default="u1")
args = parser.parse_args()
args = vars(args);

samples = np.fromfile(args['iq-file'], dtype=args['format'])
T = args['period'];
i = 0
# T = 0.015
print("Connecting...");
ctx = zmq.Context();
sock_tx = ctx.socket(zmq.PUB)
sock_tx.bind("tcp://127.0.0.1:49202")
print("Connected!");
while True:
    sock_tx.send(samples);
    print(f"{i}: transmitted! waiting... {T}s");
    sleep(T);
    i += 1