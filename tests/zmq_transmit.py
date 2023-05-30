import zmq
from time import sleep
import matplotlib.pyplot as plt
import numpy as np
import scipy
import commpy as cp
from bitarray import bitarray

from crccheck.crc import Crc16CcittFalse
from hexdump import hexdump
import commpy as cp

def format_hex(hex_str):
    out_str = "";
    for i in range(len(hex_str)):
        out_str += hex_str[i].upper();
        if i % 2 == 1:
            out_str += " ";
    return out_str;

def bpsk_modulate(msg_bytes):
    ba = bitarray(endian='big');
    ba.frombytes(msg_bytes);
    msg_bits = ba.unpack();
    nums = [];
    for i in range(len(msg_bits)):
        bit = msg_bits[i];
        nums.append(bit);
    GRAY_CODE = [0, 1]; ARITY = len(GRAY_CODE);
    indices = np.array(list(map(lambda x: GRAY_CODE[x], nums)));
    samples = np.exp(1.j * (indices / ARITY) * 2 * np.pi);
    return samples;

# def opsk_modulate(msg_bytes):
#     ba = bitarray(endian='big');
#     ba.frombytes(msg_bytes);
#     msg_bits = ba.unpack();
#     nums = [];
#     for i in range(0, len(msg_bits), 3):
#         bits = msg_bits[i:i+3];
#         num = int(bits.to01(), 2);
#         nums.append(num);
#     # print("Nums:", nums);
#     GRAY_CODE = [0, 1, 3, 2, 6, 7, 5, 4]; ARITY = len(GRAY_CODE);
#     indices = np.array(list(map(GRAY_CODE.index, nums)))
#     samples = np.exp(1.j * (indices / ARITY) * 2 * np.pi);
#     return samples;

PREAMBLE_SYNC = [0b01010101] * 3 + [0xe1, 0x5a, 0xe8, 0x93];

def construct_frame(data):
    char_list = list(PREAMBLE_SYNC);
    mlen = len(data);
    # print(hex(mlen))
    char_list.append((mlen >> 8) & 255);
    char_list.append(mlen & 255);
    char_list.append((mlen >> 8) & 255);
    char_list.append(mlen & 255);
    char_list.extend(list(data));
    char_list.extend(list(Crc16CcittFalse.calcbytes(data)));

    print("FRAME:");
    hexdump(bytes(char_list));

    samples = bpsk_modulate(bytes(char_list));
    # samples = opsk_modulate(bytes(char_list));
    samples = np.exp(1.j * np.cumsum(np.angle(samples)));
    return samples;

# socket expects 768kHz
# samples is currently at 768kHz(
# Say, 2 sps!









# One second



# def receiver():
#     ctx = zmq.Context()
#     sock = ctx.socket(zmq.SUB)
#     sock.connect("tcp://127.0.0.1:49200")
#     sock.subscribe("")
#     while True:
#         message = sock.recv()
#         print(message);

# # Create receiver process
# from multiprocessing import Process
# p = Process(target=receiver)
# p.start()
# p.join()

ctx = zmq.Context()
sock_tx = ctx.socket(zmq.PUB)
sock_tx.bind("tcp://127.0.0.1:49202")


T = 0.05;
txx = np.zeros(int(T * 768 * 1000), dtype='complex64');

i = 0
# T = 0.015
while True:
    num = list(bytes(2));
    num[0] = (i >> 8) & 0xff;
    num[1] = i & 0xff;
    
    # Convolve window

    fig, axs = plt.subplots(3);


    # transmit 20 +  bytes
    payload = [0] * 20;
    samples = construct_frame(b'\xCA\xFE\x44\xC0\xFF\xEE\xCC' + bytes(num) + bytes(payload));
    
    axs[0].plot(np.arange(samples.size), samples)
    
    SPS = 2;
    tx_samples = np.zeros(samples.size*SPS, dtype='complex64');
    tx_samples[::SPS] = samples;
    N = 102;
    fs = 48000;
    ts, h_rc = cp.filters.rcosfilter(N=N, alpha=0.6, Ts=(SPS/fs), Fs=fs);
    tx_samples = np.convolve(tx_samples, h_rc);
    # Upsample (Interpolate)
    axs[1].plot(np.arange(tx_samples.size), tx_samples, '-+')

    tx_samples = scipy.signal.resample_poly(tx_samples, 768, 48);
    axs[2].plot(np.arange(tx_samples.size), tx_samples, '-+')
    plt.show()
    
    txx[:tx_samples.size] = tx_samples;


    sock_tx.send(txx);
    sleep(T)
    i += 1


