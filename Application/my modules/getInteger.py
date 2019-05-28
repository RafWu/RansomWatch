def getInteger(integer):
    retval = 0
    try:
        retval = int(integer)
    except:
        retval = -1
    return retval