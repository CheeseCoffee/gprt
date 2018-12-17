import os
import numpy as np
import matplotlib.pyplot as plt

root = 'C:/Dev/Projects/Kafedra/output'

# list files
files = [f for f in os.listdir(root) if not os.path.isfile(f)]
for f in files:
    print("[{}] {}".format(files.index(f), f))

fi = int(input())

# do something
folder = root + os.sep + files[fi]
file = open(folder + os.sep + 'progression.txt', 'r')

gases_count = 7


def move_index(gi, i):
    return 1 + gi * 4 + i


iterations = []
flows = [[] for i in range(gases_count)]
pressures = [[] for i in range(gases_count)]

for row in file:
    data = row.strip().split(' ')
    iterations.append(int(data[0]))
    for gi in range(gases_count):
        flows[gi].append(float(data[move_index(gi, 2)]))
        pressures[gi].append(float(data[move_index(gi, 3)]))

average_flow = [[] for i in range(gases_count)]
average_pressure = [[] for i in range(gases_count)]
for gi in range(gases_count):
    average_flow[gi] = np.average(flows[gi][-100:])
    average_pressure[gi] = np.average(pressures[gi][-100:])

for gi in range(gases_count):
    print('Average flow (gas {}) = {}'.format(gi, average_flow[gi]))

for gi in range(gases_count):
    print('Average pressure (gas {}) = {}'.format(gi, average_pressure[gi]))

plt.figure(figsize=(12, 8))

for gi in range(gases_count):
    plt.subplot(2, gases_count, 1 + gi)
    plt.plot(iterations, flows[gi])
    plt.plot(iterations, [average_flow[gi] for i in range(len(iterations))])

for gi in range(gases_count):
    plt.subplot(2, gases_count, 1 + gases_count + gi)
    plt.plot(iterations, pressures[gi])
    plt.plot(iterations, [average_pressure[gi] for i in range(len(iterations))])


# plt.subplot(313)
# plt.plot(iterations, flows2)
# plt.plot(iterations, [average_flow2 for i in range(len(iterations))])

# plt.plot(iterations, [average_coef for i in range(len(iterations))])

plt.show()
