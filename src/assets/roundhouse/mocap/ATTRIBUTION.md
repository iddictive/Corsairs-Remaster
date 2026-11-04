# CMU motion capture

Subject 87, trial 01, from the [Carnegie Mellon University Graphics Lab Motion Capture Database](http://mocap.cs.cmu.edu/). The database asks for this acknowledgement: "The data used in this project was obtained from mocap.cs.cmu.edu. The database was created with funding from NSF EIA-0196217."

`87.asf` and `87_01.amc` are the original capture; `87_01.bvh` is its converted, retained authoring input. Conversion used [thcopeland/amc2bvh](https://github.com/thcopeland/amc2bvh) revision `f89c731`, with missing Euler channels initialized to zero. This patch prevents uninitialized values for channels absent from the source skeleton.
