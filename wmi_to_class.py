#!/bin/env python
#
# Convert a sounding file from Weather Modification, Inc. (WMI) sounding
# software into an old-style CLASS format sounding, creating a file with
# the same name and an extension of ".cls"
#
import sys
import time
import os

class SoundingPoint:
    def __init__(self, list):
        if (len(list) != 16):
            raise BadSoundingPoint, 'bad sounding point'
        # 0 and 1 contain minutes and seconds into sounding
        self.elapsedTime = int(list[0]) * 60 + int(list[1]);

        # 2 and 3 are latitude and longitude
        self.latitude = float(list[2])
        self.longitude = float(list[3])

        # 5 is MSL altitude (in 100s of feet); we convert to m
        self.altitude = float(list[5]) * 30.48

        # 7 is pressure (hPa)
        self.pressure = float(list[7])

        # 8 is temperature (deg C)
        self.temperature = float(list[8])

        # 9 is RH (%)
        self.rh = float(list[9])

        # 10 is dewpoint (deg C)
        self.dp = float(list[10])

        # 11 is wind direction (deg)
        self.wdir = float(list[11])

        # 12 is wind speed (kts); we convert to m/s
        self.wspd = float(list[12]) * 0.51444

class WMI_to_CLASS:

    def main(self):
        if len(sys.argv) < 2:
            sys.exit('Usage: ' + sys.argv[0] + ' <wmi_file> ...')

        for file in sys.argv[1:]:
            self.dofile(file)

    def dofile(self, filename):
        root, ext = os.path.splitext(os.path.basename(filename))
        outfilename = root + '.cls'
        try:
            infile = open(filename, 'r')
        except IOError, message:
            print str(message) + '; file skipped'
            return

        inBody = False
        points = []
        for line in infile.readlines():
            # trim leading and trailing whitespace from the line, and
            # get a list of whitespace-separated items in the line
            line = line.strip()
            items = line.split()
            
            # skip empty lines
            if (len(items) == 0):
                continue

            #
            # Before we're in the body of the sounding: check to see if we're
            # about to enter the body, and look for sounding time, location,
            # and station name information
            #
            if not inBody:
                # Lines following the 15-element line "min  s  deg   deg ..."
                # are the body of the sounding
                if (len(items) == 15 and ' '.join(items[0:3]) == 'min s deg'):
                    inBody = True
                    continue
                
                # Station : <num> <name>
                if (items[0] == 'Station'):
                    stationName = items[3]
                    continue
                
                # Location : <lat> <N|S> <lon> <E|W> <alt> m
                if (items[0] == 'Location'):
                    latitude = float(items[2])
                    if (items[3] == 'S'):
                        latitude = latitude * -1

                    longitude = float(items[4])
                    if (items[5] == 'W'):
                        longitude = longitude * -1

                    altitude = int(items[6])
                    
                    continue

                # Start Up Date <day> <month> <2-digit year> <hour:minute> <tz>
                if (' '.join(items[0:3]) == 'Start Up Date'):
                    # parse out the time
                    sndTime = time.strptime(' '.join(items[3:]),
                                            '%d %b %y %H:%M %Z')
                    continue
            #
            # Parse from a body line: elapsed time (s), lon (deg), lat (deg),
            # alt (m), pressure (hPa), temp (deg C), dewpoint (deg C), RH (%),
            # wind speed (m/s), wind direction (deg)
            #
            else:
                try:
                    points.append(SoundingPoint(items))
                except:
                    break # end of the body, skip the rest

        if (len(points) == 0):
            print 'No good points in "' + filename + '"; skipping it'
            return
        
        print 'writing', str(len(points)), 'points to "' + outfilename + '"'
        # Open the output file
        try:
            outfile = open(outfilename, 'w')
        except IOError, message:
            print 'Failed to create output file:', str(message)
            return

        #
        # Write the output file
        #
        
        # <station>,<lon>,<lat>,<alt>
        outfile.write(','.join([stationName, str(longitude),
                                str(latitude), str(altitude)]) + '\n')
        # <yyyy>,<mm>,<dd>,<hh:mm:ss>
        outfile.write(time.strftime('%Y,%m,%d,%H:%M:00', sndTime) + '\n')

        # <info>,<n_points>
        outfile.write('WMI DATA,' + str(len(points)) + '\n')

        # surface vars
        outfile.write('SURFACE:,Tim:s,Lon:d,Lat:d,Alt:m,Prs:mb,Tmp:C,Dpt:C,RH:%,WS:m/s,WD:d\n')

        # flight vars
        outfile.write('FLIGHT:,Tim:s,Lon:d,Lat:d,Alt:m,Prs:mb,Tmp:C,Dpt:C,RH:%,WS:m/s,WD:d\n')

        # rest of the header
        outfile.write('XXX\n')
        outfile.write('/\n')
        outfile.write('/\n')
        outfile.write('/\n')

        # sounding points
        for point in points:
            outfile.write(str(point.elapsedTime) + ',')
            outfile.write(str(point.longitude) + ',')
            outfile.write(str(point.latitude) + ',')
            outfile.write(str(point.altitude) + ',')
            outfile.write(str(point.pressure) + ',')
            outfile.write(str(point.temperature) + ',')
            outfile.write(str(point.dp) + ',')
            outfile.write(str(point.rh) + ',')
            outfile.write(str(point.wspd) + ',')
            outfile.write(str(point.wdir))
            outfile.write('\n')
        

if __name__ == '__main__': WMI_to_CLASS().main()
