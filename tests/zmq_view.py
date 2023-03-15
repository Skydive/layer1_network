import numpy as np
import matplotlib.pyplot as plt

data = np.fromfile("zmq_analyse.cf32", dtype='complex64');
# samples = data[0::2] + 1.j * data[1::2];

plt.figure();
plt.plot(np.arange(data.size), data, '-+');
plt.xlim(-10, 300);
plt.show();

def bpsk_demodulate(samples):
    bits = (np.real(samples) > 0).astype(int);
    bits = (bits[1:] - bits[0:-1]) % 2;
    bits = bits.astype('u1');
    return bits;

bits = bpsk_demodulate(data);

SYNC = [1,1,1,0,0,0,0,1,0,1,0,1,1,0,1,0,1,1,1,0,1,0,0,0,1,0,0,1,0,0,1,1];
SYNC = np.array(SYNC, dtype='u1');
# Take autocorrelation
corr = np.correlate(bits, SYNC, 'same');

plt.figure();
plt.plot(np.arange(corr.size), corr, '-+');
plt.show();
