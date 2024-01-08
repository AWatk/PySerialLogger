import os
import sys
import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from datetime import datetime
import time
from multiprocessing import Process, Queue, Lock, Event
import queue
import re
from collections import defaultdict
import tkinter as tk
from timeit import default_timer as timer

def runGraph(d,maxLength,timeUnits, lock,dataEnableQ, dataLabelQ):
    # define and adjust figure
    fig, ax = plt.subplots()
    data = defaultdict(list)
    filteredData = {}
    dataState = {}
    tLabel = 'Time(' + timeUnits + ')'
    dataState[tLabel] = True   
    

    def updateLabelState():
        while not dataEnableQ.empty():
            entry = dataEnableQ.get()
            name = entry[0]
            state = entry[1]
            dataState[name] = state
            if not state and name in data:
                del data[name]

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

                # check if label is a new entry and update dataState
                if label not in dataState:
                    dataState[label] = True
                    dataLabelQ.put(label)
                
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
            
            # if label is enabled
            filteredData.clear()
            for label in data:
                if dataState[label]:
                    filteredData[label] = data[label]
    def animate(i):
        updateLabelState()
        newdata = dumpQueue()
        if newdata:
            processData(newdata)
        plt.clf()
        num_subplots = len(filteredData) - 1
        if num_subplots <= 0:
            num_subplots = 1
        labels = sorted(list(set(list(filteredData.keys()))-set([tLabel])))
        gs = plt.GridSpec(num_subplots,1, height_ratios=[1]*num_subplots)  # Create a grid with equal width
        for i in range(0,len(labels)):
            ax = plt.subplot(gs[i,0])
            t = filteredData[tLabel]
            y = filteredData[labels[i]]
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

def serialProcessor(port,baudRate,logDir,d,inputQ, outQ, errQ,lock, startChar, endChar):
    sys.stdout = StdoutRedirector(outQ)
    sys.stderr = StdoutRedirector(errQ)
    curTime = datetime.now().strftime("%Y_%m_%d-%H_%M_%S")
    if not os.path.exists(logDir):
        os.makedirs(logDir)
    allFilename = logDir + '/Log_All_' + curTime + '.txt'
    dataFilename = logDir + '/Log_Data_' + curTime + '.txt'
    dataStrRegex = re.compile(re.escape(startChar) + r"(.*)" + re.escape(endChar))
    with open(allFilename,'w') as af:
        with open(dataFilename, 'w') as df:
            ser = serial.Serial(port,baudRate)
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

def monitorInput(fn,inputQ):
    sys.stdin = os.fdopen(fn)
    while True:
        inputQ.put(input())
        print("Input Received")

class StdoutRedirector(object):
    def __init__(self, queue):
        self.output = queue

    def write(self, string):
        self.output.put(string)

    def flush(self):
        pass
        

def createGui():
    exitEvent = Event()
    processes = []
    inputQ = Queue()
    outputQ = Queue()
    errQ = Queue()
    dataEnableQ = Queue()
    dataLabelQ = Queue()
    labels = []

    def StartProcesses():
        logDirectory = logs_entry.get()
        plotMaxLength = int(length_entry.get())
        timeUnits = timeUnits_entry.get()
        startChar = startChar_entry.get()
        endChar = endChar_entry.get()
        port = comPort_entry.get()
        baudRate = int(baudRate_entry.get())

        inputFn = sys.stdin.fileno()

        d = Queue()
        
        lock = Lock()
        processes.append(Process(target=runGraph,args=(d,plotMaxLength,timeUnits,lock,dataEnableQ,dataLabelQ)))
        processes.append(Process(target=serialProcessor, args=(port, baudRate,logDirectory,d,inputQ,outputQ,errQ,lock,startChar,endChar)))
        processes.append(Process(target=monitorInput, args=(inputFn,inputQ)))
        for p in processes:
            p.start()
        root.after(0, Update)

    def EndProcesses(*args):
        if not exitEvent.is_set():
            exitEvent.set()
            for p in processes:
                p.terminate()

    def Update():
        console.configure(state=tk.NORMAL)
        #while not outputQ.empty():
        try:
            console.insert('end',outputQ.get_nowait())
        except queue.Empty:
            pass
        #while not errQ.empty():
        try: 
            console.insert('end',errQ.get_nowait(), 'error')
        except queue.Empty:
            pass
        console.see('end')
        console.configure(state=tk.DISABLED)

        while not dataLabelQ.empty():
            label = dataLabelQ.get()
            labelVar = tk.IntVar()
            labelCheckButton = tk.Checkbutton(root, text=label, variable=labelVar, onvalue=1, offvalue=0, command=lambda label=label, labelVar=labelVar: SendDataState(label,labelVar))
            labelCheckButton.select()
            labels.append((labelCheckButton))
            labelCheckButton.pack()

        root.after(1,Update)
    
    def SendDataState(label, labelVar):
        dataEnableQ.put((label,labelVar.get()==1))

    def sendInput():
        inputQ.put(inputCon.get())
        inputCon.delete(0,'end')

    # create the main window
    root = tk.Tk()
    root.title("Serial Logger")

    # create labels and intries
    logs_label = tk.Label(root, text="Name of log directory")
    logs_label.pack()
    logs_entry = tk.Entry(root)
    logs_entry.insert(0,'logs')
    logs_entry.pack()

    length_label = tk.Label(root, text='Plot max length')
    length_label.pack()
    length_entry = tk.Entry(root)
    length_entry.insert(0,'200')
    length_entry.pack()

    timeUnits_label = tk.Label(root, text='Plot time units')
    timeUnits_label.pack()
    timeUnits_entry = tk.Entry(root)
    timeUnits_entry.insert(0,'ms')
    timeUnits_entry.pack()

    startChar_label = tk.Label(root, text='Start Char')
    startChar_label.pack()
    startChar_entry = tk.Entry(root)
    startChar_entry.insert(0,'~')
    startChar_entry.pack()

    endChar_label = tk.Label(root, text='End Char')
    endChar_label.pack()
    endChar_entry = tk.Entry(root)
    endChar_entry.insert(0,'~')
    endChar_entry.pack()

    comPort_label = tk.Label(root, text='Serial Port')
    comPort_label.pack()
    comPort_entry = tk.Entry(root)
    comPort_entry.insert(0,'COM5')
    comPort_entry.pack()

    baudRate_label = tk.Label(root, text='Baud Rate')
    baudRate_label.pack()
    baudRate_entry = tk.Entry(root)
    baudRate_entry.insert(0,'115200')
    baudRate_entry.pack()

    #console = tk.Text(root, height=2.5, width = 30, bg="light cyan", state=tk.NORMAL)
    console = tk.Text(root)
    console.tag_configure('error', foreground='red')
    console.pack()

    inputCon = tk.Entry(root)
    inputCon.pack()

    
    # create start button
    start_button = tk.Button(root, text="Start", command=StartProcesses)
    start_button.pack()
    input_button = tk.Button(root, text="Send", command=sendInput)
    input_button.pack()

    root.bind('<Destroy>', EndProcesses)
    

    return root    

if __name__ == '__main__':
    
    root = createGui()
    root.mainloop()
            
    
    