# 2D-LUTs
 _A detailed overview of 2D LUTs and implementing them to an AE Plugin_


## What is a LUT?

_A Look-Up Table is a set of pre-computed data which can make your code faster_

##### Instead of calculating a complex algorithm for every pixel, we can precompute data, and search for an index instead of algorithms

##### Usually a LUT is organised int a file in rows/columns, and the depth and complexity of this is based on the type of LUT

##### When dealing with Adobe programs, we are usually referring to Colour Correction/Grading LUTs, which in one form or another are effects or files you apply to clips. This can be a .lut or .cube file

##### In the realm of these LUTs (which this tutorial is primarily about), there are 2D and 3D LUTs. A 2D LUT is one which has an input value, processed using an algorithm, into an output value. A 3D lut takes 3 input values, processes them into a single index, and process them into a 3 outputs. 

##### I will discuss all about 3D LUTs in a future tutorial, and show you how complex and powerful they are.

##### The curves adjustment is a visual representation of a 2D LUT, with the Y axis being the input intensity, and the X being the output intensity for any given channel

##### A straight line (the default) can represented by the equation y = x

#### We can use other mathematical equations to create different curves and appearances
### y = x^2
<img src="https://i.imgur.com/Gak7lIO.png" />

### y = x^3
<img src="https://i.imgur.com/9xB4Pv8.png" />

### y = sqrt(x)
<img src="https://i.imgur.com/aYRe0aQ.png" />
        
##### The input pixel's intensity (let's say the rgb intensity/luma) is x. You can plugin for x, solve for y, and have your output
### y = 32^2
### y = 32 * 32
### y = 1024;

_This is far beyond the 0-255 range, so we clamp it back between (0-255)_
        
##### Using 8/16 bits is simple, with ranges of 0-255 and 0-32678

##### But 32 bpc, being 0.0-1.0 causes issues when doing squares and square roots. For example, say out input is .5

### y = .5 * .5 = 0.25

##### This means that your input would half, instead of being squared. To get around this, when using 32 bit pixels, multiply them by 255 or 32678 before applying your calculations. Then after, divide to commutate back to the original range. There's also no need to clamp in 32bpc most of the time, since it will allow you have overbr /ights
 
```
        y = (.5*255)*(.5*255)
        y = 16256.25 / 255 = 63.75
        // 63.75 is VERY br /ight, maybe you want to reduce that a bit
```
##### All that math is what is pre-computed into the LUT file, so you don't have to compute these heavy mathematical equations millions of times per image. Instead we will calculate what the output should be for every possible input. We need to pre-decide how many different values we use. 

##### For example, we could have a lut file with 255 rows, each one containing a step from black -> white (or lowest to highest intensity)

```
        r[0] = 0.0;
        r[1] = 0.0;
        r[2] = 0.05;
        r[3] = 0.08;
        r[4] = 0.1;
        r[5] = 0.1;
        //...
        //...
        //...
        r[254] = 1.0;
```
##### You could use as many steps between the min and max of your chosen range ([0-255], [0-32678], [0.0-1.0])

##### Once again the 32bpc range is a bit different. With 8/16 bits [255/32678], each step is an integer, so the max number you see is the number of steps. But with 1.0 being a float, the number of steps is ultimate up to you.

##### You could have 100000000000000000000000000000 steps between 0.0->1.0, but at a certain level of detail you are beyond the limitations of the colour precision of After Effects, monitors, and your eyes. 

##### The majority of 3D LUTs that are for colour correction, are 64^3, meaning there are 262,144 steps between the min and max of provided inputs

### For 32bpc 2D LUTs range examples
```
            100         steps = 1/100           = .01
            1000        steps = 1/1000          = .001
            10000       steps = 1/10000         = .0001
            100000      steps = 1/100000        = .00001
            1000000     steps = 1/1000000       = .000001
            10000000    steps = 1/10000000      = .0000001
            100000000   steps = 1/100000000     = .00000001
            1000000000  steps = 1/1000000000    = .000000001
```

## Creating a 2D LUT file
#### Creating a LUT file is fairly straightforward, and a fun excercise of automation

##### One cool thing is that a with LUTs formatted in this way, is the input value is implied through the row of the LUT file. The first row in our LUT file is our minimum input's result (in the case of colour correction, a black input; 0 red, 0 green, and 0 blue), the last row in our LUT file is our max input result (white), and the number of rows, is the number of steps

##### lets write the LUT file

```JavaScript
var lutFile = File("~/Documents/myLUT.cube");

var numSteps = 100000;

var lutData = [];
for(var i = 0; i < numSteps; i++) {
    lutData.push(equation(i));
}

function equation(input) {
    // divide by 255 to get the 32bpc equivalent
    // because the inputs will be 0 - 254
    return clamp(Math.sqrt(input))/255.0;
}

function clamp(input) {
    if(input > 255) return 255;
    if(input < 0) return 0;
    return input;
}

lutFile.open("w");
lutFile.write(lutData.join("\r\n"));
lutFile.close();
```

#### Now our file looks like:
```
0
0.00392156862745
0.00554593553872
0.00679235610811
0.0078431372549
0.00876889402941
0.00960584212856
0.01037549533751
0.01109187107744
0.01176470588235
0.01240108886341
0.01300637172688
...
...
...
...
...
...
1
1
1
1
1
```


##### We have now precomputed the square root of the input 100 thousand times. This may not seem like a lot of time saved, but if you are doing large amounts of iterations or a much more complex algorithm (which is common in higher order LUTs), the time saved can save ample computation time. 

## Reading our LUT file

##### Lastly, we need to be able to read this data back in. Let's say at this point you've automated the creation of 100 2D LUT files, and you want to be able to read them back in. This is where it could get fun. The possibilities for a 2D LUT are seemingly limitless. The input could be anything, a slider value, a pixel's luminance, a pixel's red intensity, anything that has a single dimensional value. For the purposes of this, let's assume our LUT is to adjust the look of an image. We can apply the single dimensional output for any given input, to the R, G, and B channels to apply our LUT.

##### Let's do it with a plugin:

#### *Remember* - The row # is the input value. If we have a 32bpc pixel, and we calculate the red channel intensity is .5 (50% intensity), there is no row .5

##### We need to interpolate the value of our pixel input range, to our LUT range:
```
input/inputRangeMax*10000 
floor(.5/1.0*10000)
= 5000
````

**(for safety we should floor or round. Rows are ints and we will use this value as an index)**

##### This means, for the input value of .5, we should reference row 5000 of our LUT file for the output file

##### If we create an array that holds each row of our LUT file in each of its indicies, we can apply our LUT with basically 1 line of code (per channel)

```
applyLUT() {
    var lutFile = File();

    float LUTArrayData[100000] = lutFile.read().split("\r\n");

    outP->red = LUTArrayData[Math.floor(inP->red/1.0*10000)];
    outP->green = LUTArrayData[Math.floor(inP->green/1.0*10000)];
    outP->blue = LUTArrayData[Math.floor(inP->blue/1.0*10000)];
    outP->alpha = 1.0;
}
```

##### This applies the same 2D LUT to each of our 3 colour channels, but there are many variations and fun things you can do. Using different equations for different channels is essentially like using the curves effect and changing the various individual channels

##### There is an additional level of detail we can add. In this case, we are flooring our input value, to make sure it is a valid integer for our LUT array. But if we are flooring it, we are forcing it to be a data value less precise than it should be. 

##### For example, if our input value gives us row 5000.54321, although this isn't a valid row, neither is row 5000 or 5001 technically accurate for this result. The solution is interpolation. If row 5000 gives an a precomputed output of .71 and row 5001 .735, we can interpolate between the min and max (.71 and .735), at the point our 5000.54321 input would be. This would look like this:

```
applyLUT() {
    var lutFile = File();

    float LUTArrayData[100000] = lutFile.read().split("\r\n");

    if(Math.floor(inP->red/1.0*10000) != inP->red/1.0*10000) {
        // if our floor doesn't equal the non floored input, we interpolate, else, use the floored value
        outP->red = (LUTArrayData[Math.floor(inP->red/1.0*10000)] + LUTArrayData[Math.floor(inP->red/1.0*10000)+1]) / 2;
    } else {
        outP->red = LUTArrayData[Math.floor(inP->red/1.0*10000)];
    }
    
    if(Math.floor(inP->green/1.0*10000) != inP->green/1.0*10000) {
        // if our floor doesn't equal the non floored input, we interpolate, else, use the floored value
        outP->green = (LUTArrayData[Math.floor(inP->green/1.0*10000)] + LUTArrayData[Math.floor(inP->green/1.0*10000)+1]) / 2;
    } else {
        outP->green = LUTArrayData[Math.floor(inP->green/1.0*10000)];
    }


    if(Math.floor(inP->blue/1.0*10000) != inP->blue/1.0*10000) {
        // if our floor doesn't equal the non floored input, we interpolate, else, use the floored value
        outP->blue = (LUTArrayData[Math.floor(inP->blue/1.0*10000)] + LUTArrayData[Math.floor(inP->blue/1.0*10000)+1]) / 2;
    } else {
        outP->blue = LUTArrayData[Math.floor(inP->blue/1.0*10000)];
    }
    outP->alpha = 1.0;
}
```

#### All the above C++ code was me writing some experimental code, you can see the attached 2D LUT plugin and source code to see the full code

## AE Plugin

#### You can update the hardcoded LUT file path, install the plugin, and apply it to a layer by going to Effect -> NT Productions -> 2D LUT

### Before and After

<img src="https://i.imgur.com/nLFvGfR.png" title="before" />

<img src="https://i.imgur.com/rCYrx9S.png" title="after" />