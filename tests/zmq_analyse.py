import zmq
import time
import numpy
import matplotlib.pyplot as plt

import numpy as np
from scipy.signal import resample, resample_poly, butter, filtfilt
from bitarray import bitarray

def find_kmp(seq, subseq):
    n = len(seq)
    m = len(subseq)
    # : compute offsets
    offsets = [0] * m
    j = 1
    k = 0
    while j < m: 
        if subseq[j] == subseq[k]: 
            k += 1
            offsets[j] = k
            j += 1
        else: 
            if k != 0: 
                k = offsets[k - 1] 
            else: 
                offsets[j] = 0
                j += 1
    # : find matches
    i = j = 0
    while i < n: 
        if seq[i] == subseq[j]: 
            i += 1
            j += 1
        if j == m:
            yield i - j
            j = offsets[j - 1] 
        elif i < n and seq[i] != subseq[j]: 
            if j != 0: 
                j = offsets[j - 1] 
            else: 
                i += 1

def mm_sync(samples, sps):
    mu = 0 # initial estimate of phase of sample
    out = np.zeros(len(samples) + 10, dtype='complex64')
    out_rail = np.zeros(len(samples) + 10, dtype='complex64') # stores values, each iteration we need the previous 2 values plus current value
    i_in = 0 # input samples index
    i_out = 2 # output index (let first two outputs be 0)
    while i_out < len(samples) and i_in+16 < len(samples):
        out[i_out] = samples[i_in + int(mu)] # grab what we think is the "best" sample
        out_rail[i_out] = int(np.real(out[i_out]) > 0) + 1j*int(np.imag(out[i_out]) > 0)
        x = (out_rail[i_out] - out_rail[i_out-2]) * np.conj(out[i_out-1])
        y = (out[i_out] - out[i_out-2]) * np.conj(out_rail[i_out-1])
        mm_val = np.real(y - x)
        mu += sps + 0.3*mm_val
        i_in += int(np.floor(mu)) # round down to nearest int since we are using it as an index
        mu = mu - np.floor(mu) # remove the integer part of mu
        i_out += 1 # increment output index
    out = out[2:i_out] # remove the first two, and anything after i_out (that was never filled out)
    return out;

def format_hex(hex_str):
    out_str = "";
    for i in range(len(hex_str)):
        out_str += hex_str[i].upper();
        if i % 2 == 1:
            out_str += " ";
    return out_str;

def bpsk_demodulate(samples):
    bits = (np.real(samples) > 0).astype(int);
    bits = (bits[1:] - bits[0:-1]) % 2;
    bits = bits.astype('u1');
    return bits;

def process_samples(samples):
    # sps = 2
    # line1.set_xdata(np.arange(0, len(samples)));
    # line1.set_ydata(samples);
    # axs[0].set_xlim(-100, len(samples)+100)

    fs = 768e3;
    normal_cutoff = 25e3 / (fs/2);
    # Get the filter coefficients 
    b, a = butter(2, normal_cutoff, btype='low', analog=False)

    line0.set_xdata(np.arange(0, len(samples)));
    line0.set_ydata(samples);
    axs[0].set_xlim(-100, len(samples)+100)

    samples = filtfilt(b, a, samples)

    line1.set_xdata(np.arange(0, len(samples)));
    line1.set_ydata(samples);
    axs[1].set_xlim(-100, len(samples)+100)

    samples = samples[::4];
    # samples = resample(samples, 48, 768);

    # samples = resample_poly(samples, 384, 768); # @ 16SPS
    samples = mm_sync(samples, 8);

    # samples = mm_sync(samples, 2);

    # line1.set_xdata(np.arange(0, len(samples)));
    # line1.set_ydata(samples);
    # axs[1].set_xlim(-100, len(samples)+100)


    line2.set_xdata(np.real(samples))    
    line2.set_ydata(np.imag(samples))

    # Do DBPSK demodulation
    bits = bpsk_demodulate(samples);

    SYNC = [1,1,1,0,0,0,0,1,0,1,0,1,1,0,1,0,1,1,1,0,1,0,0,0,1,0,0,1,0,0,1,1];
    SYNC = np.array(SYNC, dtype='u1');
    # Take autocorrelation
    corr = np.correlate(bits, SYNC, 'same');
    line3.set_xdata(np.arange(len(corr)));
    line3.set_ydata(corr);
    axs[3].set_xlim(-2, len(corr)+2)
    axs[3].set_ylim(-1, np.max(corr)+2);
    
    # SYNC BYTES

    # FIND MATCH
    kmp_list = list(find_kmp(bits, SYNC))
    # print("KMP:", kmp_list, len(SYNC));
    if(len(kmp_list) > 0):
        kmp_idx = kmp_list[0];
        bits = bits[kmp_idx+len(SYNC):]
        # DATA!
        ba = bitarray(endian='big')
        ba.pack(bytes(list(bits)))
        print(format_hex(ba.tobytes().hex()))
        process_bytes(ba.tobytes())

    # print("FLUSH CANVAS")
    fig.canvas.draw();
    fig.canvas.flush_events();

from crccheck.crc import Crc16CcittFalse
def process_bytes(msg_bytes):
    mlen1 = msg_bytes[0:2];
    mlen2 = msg_bytes[2:4];
    if(mlen1 != mlen2):
        print("Length header corrupted, Frame dropped");
        return;
    count = mlen1[1] + (mlen1[0] << 8);
    print("mLen: ", count);
    msg_bytes_crc = msg_bytes[4+count:4+count+2];
    msg_bytes = msg_bytes[4:4+count];
    calc_crc = Crc16CcittFalse.calcbytes(msg_bytes);
    if(calc_crc != msg_bytes_crc):
        print("CRC check failed: Message dropped");
        return;
    print("Message: ", format_hex(msg_bytes.hex()))
    
ctx = zmq.Context()
sock = ctx.socket(zmq.SUB)
# sock.connect("tcp://127.0.0.1:49202")
sock.connect("tcp://127.0.0.1:49201")
sock.subscribe("")

N = int(0.015 * 768 * 1000)

plt.ion()
fig, axs = plt.subplots(4)

xs = np.arange(0, N);
ys = np.empty(N);

line0, = axs[0].plot(xs, ys, 'b-')
axs[0].set_ylim(-1.5, 1.5);


line1, = axs[1].plot(xs, ys, 'b-')
axs[1].set_ylim(-1.5, 1.5);

line2, = axs[2].plot(xs, xs, 'b+');
axs[2].set_xlim(-1.5, 1.5);
axs[2].set_ylim(-1.5, 1.5);

line3, = axs[3].plot(xs, xs, 'b-');

fig.canvas.draw();
fig.canvas.flush_events();


# modem = cp.modulation.PSKModem(2);

# Store a 0.02s buffer
buffer = np.empty(N, dtype='complex64');
state = False;
write_idx = 0;
while True:
    message = sock.recv()
    # print("R", end="")
    data = np.frombuffer(message, dtype='complex64')
    # ABS the data
    if not state:
        data_abs = np.abs(data);
        where_exceeds = np.where(np.abs(data) > 0.2)[0]
        if len(where_exceeds) > 0:
            # print("TRIGGER")
            trigger_idx = max(where_exceeds[0]-200, 0);
            state = True;

    if state and trigger_idx >= 0:
        frame = data[trigger_idx:];
        buffer[write_idx:write_idx+len(frame)] = frame[:min(len(frame), buffer.size - write_idx)];
        write_idx += len(frame);
        trigger_idx = -1;
    elif state and trigger_idx == -1:
        space_left = len(buffer) - write_idx;
        if(space_left > 0):
            frame = data[:space_left];
            buffer[write_idx:write_idx+len(frame)] = frame;
            write_idx += len(frame);
            # print("Writing: ", len(frame), write_idx, len(buffer));

    if(write_idx >= len(buffer)):
        samples = buffer;

  
        process_samples(samples);

        state = False;
        write_idx = 0;
        # print("NEXT LOOP")
        # break;

# Do demodulation
# fig, axs = plt.subplots(3);
# axs = axs.flatten();

# sps = 2
# samples = scipy.signal.resample_poly(buffer, 48, 768);
# samples_sync = mm_sync(samples, sps);
# axs[0].plot(np.arange(len(buffer)), buffer);
# axs[1].plot(np.arange(len(samples)), samples);
# axs[2].plot(np.arange(len(samples_sync)), samples_sync, '+');
# fig.show()
# plt.show()
        
        # line1.set_xdata(np.arange(0, len(buffer)));
        # line1.set_ydata(np.real(buffer));


        

