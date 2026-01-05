#!/opt/homebrew/bin/python3

# this code loads tracks from a text file and save it in memory
# the text file constains tracks in the following strict format:
#     one line is for one track, one track has 6 points
#     x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, x5, y5, z5

class vec3_t:
    def __init__(self, x=0, y=0, z=0):
        self.x = x; self.y = y; self.z = z;

    def __str__(self):
        return f'vec3_t({self.x}, {self.y}, {self.z})'

    def print(self):
        print("({}, {}, {})".format(self.x, self.y, self.z));

class track_parser:
    def __init__(self, path="../generator/tracks.txt"):
        self.path = path

        with open(path) as f:
            self.cache = f.readlines()

        self.tracks = []
        for i in self.cache:
            elements = i.split()
            t = [vec3_t(float(elements[3*k]), float(elements[3*k+1]), float(elements[3*k+2])) for k in range(int(len(elements)/3))]
            self.tracks.append(t)

    def print(self):
        print(len(self.tracks))


# unit test
'''
tracks = track_parser()
tracks.print()
print(len(tracks.tracks))
'''
