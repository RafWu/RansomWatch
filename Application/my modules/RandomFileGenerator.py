import random
import os
import uuid     # random file name
import sys      # probably won't be needed in the actual app, but I keep it for now to see how the script works
import datetime

"""
TODO: random file for hidden files are also generated. hiding the generated file is only possibe via WIN API
"""
# constants
FILENAME_MIN_LENGTH = 4
FILENAME_MAX_LENGTH = 15
MAX_TRAP_SIZE = 50000000    # 50 MB
DEBUG = True

__file = None

def log(msg, rname):
    if DEBUG:
        __file.write("{}: {}\n".format(rname, msg))
    
def generateRandomFile(extension, absPath, sizeLimits):
    """
    Generates random file in a given path with a given extension
    with random size within the given limits.
    
    Arguments:
        - extension: the requested file extension (str)
        - absPath: the absolute path to which to write the new file (str)
        - sizeLimits: a tuple that defines the lower and upper limit to
          the size of file in bytes ((int, int))
    """
    rname = "generateRandomFile"
    
    filename = ""
    while(True):
        filename_length = random.randint(FILENAME_MIN_LENGTH, FILENAME_MAX_LENGTH);
        filename = str(uuid.uuid4())[:filename_length] + "." + extension
        if not os.path.isfile(absPath + "\\" + filename):
            break
    
    # randomize the number of bytes to write between the given limits
    num_bytes = random.randint(sizeLimits[0], sizeLimits[1]);
    
    # create the file
    # TODO: handle some special file extensions differently
    filename = absPath + "\\" + filename
    log("Creating file: {}".format(filename), rname)
    """
    if extension == "xls":
        generateRandomXLS(filename, num_bytes)
    elif extension == "doc":
        generateRandomDOC(filename, num_bytes)
    elif extension == "pdf":
        generateRandomPDF(filename, num_bytes)
    elif extension == "jpg":
        generateRandomJPG(filename, num_bytes)
    else:
    """
    try:
        with open(filename, 'wb') as f:
            f.write(os.urandom(num_bytes))
    except IOError as e:
        log("I/O error({}): {}.".format(e.errno, e.strerror), rname)
        return
    except:
        log("Unexpected error.", rname)
        return
    
    log("Done generating a random file.", rname)
    return filename


def chooseSize(ext, path, dirs):
    """
    Returns lower and upper limit of file size in certain folder
    of a specific file extension.
    
    Arguments:
        - ext: extension to examine
        - path: absolute path to the desired directory
        - dirs: files listing in the given path
    """
    
    rname = "chooseSize"
    
    min = max = 0
    for filename in dirs:
        # count only those with .ext
        if not filename.endswith("." + ext):
            continue
            
        filename = path + "\\" + filename
        if os.path.isfile(filename):
            size = os.path.getsize(filename)
            if min == 0:
                min = max = size
            elif size < min:
                min = size
            elif size > max:
                max = size

    if min > MAX_TRAP_SIZE:
        min, max = MAX_TRAP_SIZE, 2 * MAX_TRAP_SIZE
    elif max > MAX_TRAP_SIZE:
        max = MAX_TRAP_SIZE

    return (min, max)   # in case only one file in that extension,
                        # min = max. do we care?
    
    
def main(path):
    # just for now
    # TODO: handle exceptions (for now only chooseSize can throw)
    rname = "main"
    
    if DEBUG:
        global __file
        # open can throw, but that's OK...
        # error will be presented in the terminal
        __file = open('RandomFileGenerator.log', 'a')
        print("Log file is in \"{}\".".format(os.getcwd()))
        log("Log started at {}. Working on directory: {}.".format(datetime.datetime.now(), path), rname)
    
    if not os.path.isdir(path):
        log("{} is not a directory.".format(absPath), rname)
        raise NotADirectoryError
        
    exts = set()
    dirs = os.listdir(path)
    for filename in dirs:
        if filename.rfind(".") == -1 or os.path.isdir(path + "\\" + filename):
            continue

        exts.add(filename[filename.rfind(".") + 1:].lower())
        # TODO: check if ends with .

    retList = []
    for ext in exts:
        # dirs is passed for optimization only,
        # it can be avoided, but possibly will reduce preformance
        sizes = chooseSize(ext, path, dirs);
        log("(min, max) = ({}, {}) for extension {}.".format(sizes[0], sizes[1], ext), rname)
        ret = generateRandomFile(ext, path, sizes)
        retList.append(ret)

    if DEBUG:
        log("Log done.", rname)
        __file.close()
        
    return retList



if __name__ == "__main__":
    main(sys.argv[1])