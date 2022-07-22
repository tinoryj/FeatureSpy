#!/user/local/bin/python
# coding:utf-8
import re
import sys

if __name__ == '__main__':
    oldFilePath = sys.argv[1] + ".html"
    fopen = open(oldFilePath, 'r')
    writeStr1 = sys.argv[3]
    writeStr2 = sys.argv[4]
    writeOutStr = ""
    for line in fopen:
        if re.search('\$450', line):
            line = re.sub('450', writeStr1, line)
            writeOutStr += line
        else:
            if re.search('Bonus of \$300,000', line):
                line = re.sub('Bonus of \$300,000', 'Bonus of $' +
                              writeStr2+'0,000', line)
                writeOutStr += line
            else:
                writeOutStr += line
    newFilePath = sys.argv[2] + sys.argv[1] + "-" + \
        writeStr1 + "-" + writeStr2 + ".html"
    wopen = open(newFilePath, 'w')
    wopen.write(writeOutStr)
    fopen.close()
    wopen.close()
