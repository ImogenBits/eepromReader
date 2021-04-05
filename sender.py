import time
import os
import math
import sys
from pySerialTransfer import pySerialTransfer as txfer

# Print iterations progress
def printProgressBar (iteration, total, prefix = '', suffix = '', length = 100, fill = '█', printEnd = "\r"):
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
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print('\r%s |%s| %s/%s %s' % (prefix, bar, iteration, total, suffix), end = printEnd)
    # Print New Line on Complete
    if iteration == total: 
        print()

link = txfer.SerialTransfer("COM3", baud = 115200)
def main():
    try:
        instruction = sys.argv[1] if len(sys.argv) > 1 else "read"

        if instruction == "write":
            link.open()
            time.sleep(2) # allow some time for the Arduino to completely reset

            fileName = sys.argv[2] if len(sys.argv) > 2 else "EEPROMdata"
            fileStartPage = int(sys.argv[3]) if len(sys.argv) > 3 else 0
            fileEndPage = 0
            if len(sys.argv) > 4:
                if sys.argv[4] == "end":
                    fileEndPage = 2**10
                else:
                    fileEndPage = int(sys.argv[4]) + 1
            elif len(sys.argv) > 3:
                fileEndPage = fileStartPage + 1
            else:
                fileEndPage = 2**10
            eepromPageOffset = int(sys.argv[5]) if len(sys.argv) > 5 else fileStartPage
            
            fileEnd = fileEndPage * 128 if fileEndPage <= 2**10 else 2**17
            file = []
            with open(os.path.join("EEPROMFiles", fileName), "rb") as f:
                file = f.read()[(fileStartPage * 128):fileEnd]
            pageCount = math.ceil(len(file) / 128)

            printProgressBar(0, pageCount)

            for pageNum in range(pageCount):
                #initialise
                sentBytes = 0
                offset = pageNum * 128
                arraySize = 128
                if (len(file) - offset < 128):
                    arraySize = len(file) - offset
                dataArray = file[offset:(offset + arraySize)]
                
                #put data in buffer
                sentBytes = link.tx_obj(1, sentBytes)   #inserts data.instruction
                intSize = sentBytes
                sentBytes = link.tx_obj(pageNum + eepromPageOffset, sentBytes)   #inserts data.pageNum
                sentBytes = link.tx_obj(arraySize, sentBytes)   #inserts data.len
                for index, data in enumerate(dataArray): #inserts data.dara
                    link.txBuff[index + sentBytes] = chr(data)
                sentBytes += len(dataArray)

                #send data
                link.send(sentBytes)

                #wait for response
                while not link.available():
                    if link.status < 0:
                        if link.status == -1:
                            print('ERROR: CRC_ERROR')
                        elif link.status == -2:
                            print('ERROR: PAYLOAD_ERROR')
                        elif link.status == -3:
                            print('ERROR: STOP_BYTE_ERROR')
                
                #parse response
                receivedBytes = 0

                receivedPage = link.rx_obj(int, 4, receivedBytes)
                receivedBytes += 4
                
                if receivedPage != pageNum + eepromPageOffset:
                    raise ValueError("sent page: {}, returned page: {}".format(pageNum + eepromPageOffset, receivedPage))

                printProgressBar(pageNum + 1, pageCount)

                pageNum += 1
        elif instruction == "read":
            if not link.open():
                return

            time.sleep(2) # allow some time for the Arduino to completely reset
            
            startPageNum = int(sys.argv[2]) if len(sys.argv) > 2 else 0
            endPageNum = int(sys.argv[3]) + 1 if len(sys.argv) > 3 else startPageNum + 1
            
            for pageNum in range(startPageNum, endPageNum):
                sentBytes = 0
                sentBytes = link.tx_obj(2, sentBytes)   #inserts data.instruction
                intSize = sentBytes
                sentBytes = link.tx_obj(pageNum, sentBytes) #inserts data.pageNum
                sentBytes = link.tx_obj(0, sentBytes)   #inserts data.len

                link.send(sentBytes)
                
                #wait for response
                while not link.available():
                    if link.status < 0:
                        if link.status == -1:
                            print('ERROR: CRC_ERROR')
                        elif link.status == -2:
                            print('ERROR: PAYLOAD_ERROR')
                        elif link.status == -3:
                            print('ERROR: STOP_BYTE_ERROR')
                

                receivedBytes = 0

                receivedPage = link.rx_obj(int, obj_byte_size=4, start_pos=receivedBytes)
                receivedBytes += 4
                receivedLen = link.rx_obj(int, obj_byte_size=4, start_pos=receivedBytes)
                receivedBytes += 4
                receivedList = link.rx_obj(list, obj_byte_size=receivedLen, start_pos=receivedBytes, list_format="B")
                
                if receivedPage != pageNum:
                    raise ValueError("sent page: {}, received page: {}".format(pageNum, receivedPage))
                
                print("page {}:".format(receivedPage))
                for i in range(4):
                    outputGroups = ["", "", "", ""]
                    for j in range(4):
                        offset = (4 * i + j) * 8
                        outputGroups[j] =" ".join(["{0:02X}".format(num) for num in receivedList[offset:(offset + 8)]])
                    print("   ".join(outputGroups))
        elif instruction == "help":
            print("write [fileName] [fileStartPage] [FileEndPage] [EepromPageOffset]")
            print("read [startPage] [endPage]")
            





    
    except KeyboardInterrupt:
        link.close()
        print()
    
    except:
        import traceback
        traceback.print_exc()
        
        link.close()
        print()


if __name__ == "__main__":
    main()