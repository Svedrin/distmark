#!/usr/bin/python
# -*- coding: utf-8 -*-
# kate: space-indent on; indent-width 4; replace-tabs on;

import re
import sys
import json
import subprocess

from optparse import OptionParser

class PostMark(object):
    def __init__(self, location, files, transactions):
        self.postmark = None
        self.location = location
        self.files    = files
        self.transactions = transactions

    def prepare(self):
        if self.postmark is None:
            self.postmark = subprocess.Popen(["postmark"],
                stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    def run(self):
        if self.postmark is None:
            self.prepare()
        out, err = self.postmark.communicate(
            "set location %(loc)s\n"
            "set number %(fno)d\n"
            "set transactions %(ntr)d\n"
            "run\n" % { "loc": self.location, "fno": self.files, "ntr": self.transactions })
        return self.parse_result(out)

    def parse_result(self, out):
        regexlines = [
            r"PostMark (?P<version>v\d+\.\d+ : [\d/]+)",
            r"Creating files...Done",
            r"Performing transactions..........Done",
            r"Deleting files...Done",
            r"Time:",
            r"\s+(?P<totalsecs>\d+) seconds total",
            r"\s+(?P<transsecs>\d+) seconds of transactions \((?P<transpersec>\d+) per second\)",
            r"",
            r"Files:",
            r"\s+(?P<fcreated>\d+) created \((?P<fcreatedpersec>\d+) per second\)",
            r"\s+Creation alone: (?P<fcreatedalone>\d+) files \((?P<fcreatedalonepersec>\d+) per second\)",
            r"\s+Mixed with transactions: (?P<fcreatedtrans>\d+) files \((?P<fcreatedtranspersec>\d+) per second\)",
            r"\s+(?P<fread>\d+) read \((?P<freadpersec>\d+) per second\)",
            r"\s+(?P<fappend>\d+) appended \((?P<fappendpersec>\d+) per second\)",
            r"\s+(?P<fdeleted>\d+) deleted \((?P<fdeletedpersec>\d+) per second\)",
            r"\s+Deletion alone: (?P<fdeletedalone>\d+) files \((?P<fdeletedalonepersec>\d+) per second\)",
            r"\s+Mixed with transactions: (?P<fdeletedtrans>\d+) files \((?P<fdeletedtranspersec>\d+) per second\)",
            r"",
            r"Data:",
            r"\s+(?P<bytesread>[\d.]+) (?P<bytesreadunit>\w+) read \((?P<bytesreadpersec>[\d.]+) (?P<bytesreadpersecunit>\w+) per second\)",
            r"\s+(?P<byteswritten>[\d.]+) (?P<byteswrittenunit>\w+) written \((?P<byteswrittenpersec>[\d.]+) (?P<byteswrittenpersecunit>\w+) per second\)",
            ]
        outlines = out.replace("pm>", "").split("\n")
        if len(outlines) < len(regexlines):
            raise ValueError("PostMark output appears to be incomplete (expecting %d lines, got %d)" % (len(regexlines), len(outlines)))
        result = {}
        for lno, (regex, outline) in enumerate( zip(regexlines, outlines) ):
            m = re.match(regex, outline)
            if m is None:
                raise ValueError("Could not parse PostMark result line %d: %s" % (lno, outline))
            result.update(m.groupdict())
        return result

class WorkerVM(object):
    pass


def main():
    parser = OptionParser()

    parser.add_option("-t", "--test-postmark", default=None, help="Tests the postmark result parser. Pass an appropriate input file as this option's value.")

    options, posargs = parser.parse_args()

    if options.test_postmark:
        pm = PostMark(None, None, None)
        print json.dumps(pm.parse_result(open(options.test_postmark, "r").read()), indent=4)


if __name__ == '__main__':
    sys.exit(main())
