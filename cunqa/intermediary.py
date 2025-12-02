import sys, os

from cunqa.logger import logger



if __name__ == "__main__":
    print("Inside Python intermediary")
    supported_real_qpus = ["QMIO"]

    if (len(sys.argv) < 2):
        logger.error("Real QPU not specified as first argument. Aborting")
        sys.exit(1)
