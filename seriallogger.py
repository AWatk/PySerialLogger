import os
import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from datetime import datetime
import time
from multiprocessing import Process, Queue, Lock
import re
from collections import defaultdict

def runGraph(d,maxLength,timeUnits, lock):
    # define and adjust figure
    fig, ax = plt.subplots()
    tLabel = 'Time(' + timeUnits + ')'
    data = defaultdict(list)

    def dumpQueue():
        newdata = []
        with lock:
            while not d.empty():
                newdata.append(d.get())
        return newdata

    def processData(newdata):
        for pt in newdata:
            newDataLabels = []
            label, val = pt[0].split(':') # assume time is always first
            newDataLabels.append(label)
            data[tLabel].append(float(val)) # use default time label
            del data[tLabel][0:-maxLength]

            # for all label:val pairs in pt besides time
            for i in range(1,len(pt)):
                label,val = pt[i].split(':')
                newDataLabels.append(label)
                if label in data:
                    data[label].append(float(val))
                else:
                    data[label].extend([0]*(len(data[tLabel])-1))
                    data[label].append(float(val))
                del data[label][0:-maxLength]

            # if existing label not present in pt, append current value
            for label in list(set(list(data.keys()))-set(newDataLabels)): 
                data[label].append(data[label][-1])
                del data[label][0:-maxLength]

    def animate(i):
        newdata = dumpQueue()
        if newdata:
            processData(newdata)
        plt.clf()
        num_subplots = len(data) - 1
        if num_subplots <= 0:
            num_subplots = 1
        labels = sorted(list(set(list(data.keys()))-set([tLabel])))
        gs = plt.GridSpec(num_subplots,1, height_ratios=[1]*num_subplots)  # Create a grid with equal width
        for i in range(0,len(labels)):
            ax = plt.subplot(gs[i,0])
            t = data[tLabel]
            y = data[labels[i]]
            ax.plot(t,y)
            ax.set_ylabel(labels[i])
            ax.relim()
            ax.autoscale()
            if (i != (len(labels) -1)):
                ax.tick_params(labelbottom=False)
        return []

    ani = animation.FuncAnimation(fig,
        animate,
        fargs=(),
        interval=25,
        blit=False)

    plt.show()

def serialProcessor(logDir,d,inputQ,lock, startChar, endChar):
    curTime = datetime.now().strftime("%Y_%m_%d-%H_%M_%S")
    if not os.path.exists(logDir):
        os.makedirs(logDir)
    allFilename = logDir + '/Log_All_' + curTime + '.txt'
    dataFilename = logDir + '/Log_Data_' + curTime + '.txt'
    dataStrRegex = re.compile(re.escape(startChar) + r"(.*)" + re.escape(endChar))
    with open(allFilename,'w') as af:
        with open(dataFilename, 'w') as df:
            ser = serial.Serial('COM5',115200)
            ser.close()
            ser.timeout = 110
            ser.open()
            time.sleep(0.1)
            ser.flushInput()
            #ser.readline()
            while True:
                # send any user input on to the serial TX
                if(not inputQ.empty()):
                    ser.write(inputQ.get().encode("UTF-8") + b'\n')

                # get next RX line
                rxLine = ser.readline().decode().rstrip()

                #write it to All log
                af.write(rxLine + '\n')
                
                #check to see if it contains data
                result = dataStrRegex.search(rxLine)
                if result:
                    dataString = result.group(1)
                    #write to file
                    df.write(dataString + '\n')
                    
                    #split data string into tuple
                    dataTuple = dataString.split(',')
                    
                    #update shared memory array
                    with lock:
                        d.put(dataTuple)
                else:
                    print(rxLine) # if not data, print to terminal


if __name__ == '__main__':
    
    # user defined variables
    logDirectory = 'logs'
    plotMaxLength = 200
    timeUnits = 'ms'
    startChar = '~'
    endChar = '~'

    d = Queue()
    inputQ = Queue()
    exitQ = Queue()
    lock = Lock()
    p1 = Process(target=runGraph,args=(d,plotMaxLength,timeUnits,lock))
    p2 = Process(target=serialProcessor, args=(logDirectory,d,inputQ,lock,startChar,endChar))
    p1.start()
    p2.start()
    try:
        while(True):
            inputQ.put(input())
            print("Input Received")
    except KeyboardInterrupt:
            print('Exiting')
            p1.terminate()
            p2.terminate()
            p1.join()
            p2.join()
            exitQ
    
    