// Copyright (c) 2008-2011, Guoshen Yu <yu@cmap.polytechnique.fr>
// Copyright (c) 2008-2011, Jean-Michel Morel <morel@cmla.ens-cachan.fr>
//
// WARNING: 
// This file implements an algorithm possibly linked to the patent
//
// Jean-Michel Morel and Guoshen Yu, Method and device for the invariant 
// affine recognition recognition of shapes (WO/2009/150361), patent pending. 
//
// This file is made available for the exclusive aim of serving as
// scientific tool to verify of the soundness and
// completeness of the algorithm description. Compilation,
// execution and redistribution of this file may violate exclusive
// patents rights in certain countries.
// The situation being different for every country and changing
// over time, it is your responsibility to determine which patent
// rights restrictions apply to you before you compile, use,
// modify, or redistribute this file. A patent lawyer is qualified
// to make this determination.
// If and only if they don't conflict with any patent terms, you
// can benefit from the following license terms attached to this
// file.
//
// This program is provided for scientific and educational only:
// you can use and/or modify it for these purposes, but you are
// not allowed to redistribute this work or derivative works in
// source or executable form. A license must be obtained from the
// patent right holders for any other use.
//
// 
//*----------------------------- demo_ASIFT  --------------------------------*/
// Detect corresponding points in two images with the ASIFT method. 

// Please report bugs and/or send comments to Guoshen Yu yu@cmap.polytechnique.fr
// 
// Reference: J.M. Morel and G.Yu, ASIFT: A New Framework for Fully Affine Invariant Image 
//            Comparison, SIAM Journal on Imaging Sciences, vol. 2, issue 2, pp. 438-469, 2009. 
// Reference: ASIFT online demo (You can try ASIFT with your own images online.) 
//			  http://www.ipol.im/pub/algo/my_affine_sift/
/*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <dirent.h>
#include <algorithm>
using namespace std;

//#ifdef _OPENMP
#include <omp.h>
//#endif

#include "demo_lib_sift.h"
#include "io_png/io_png.h"

#include "library.h"
#include "frot.h"
#include "fproj.h"
#include "compute_asift_keypoints.h"
#include "compute_asift_matches.h"

# define IM_X 800
# define IM_Y 600

//imagefile containing the imagepixels, width, height and keypooints
struct imagefile
{
    std::vector<float> imageData;
    string framename;
    size_t w;
    size_t h;
    int haskeys;
    vector< vector< keypointslist > > keys;
};

//copy the file from inputdir to outputdir.
bool copyFile(const char *SRC, const char* DEST)
{
    std::ifstream src(SRC, std::ios::binary);
    std::ofstream dest(DEST, std::ios::binary);
    dest << src.rdbuf();
    return src && dest;
};

//read the directory, this gives an a-z sorted file list of the contents. Ignoring subdirs.
vector<string> read_directory( const std::string& path = std::string() )
{
    vector<string> result;
    dirent* de;
    DIR* dp;
    int errno;

    dp = opendir( path.empty() ? "." : path.c_str() );
    if (dp)
    {
        while (true)
        {
            errno = 0;
            de = readdir( dp );
            if (de == NULL) break;
            if (de->d_type == 0x4) continue; //0x4 = dir
            result.push_back( std::string( de->d_name ) );
        }
        closedir( dp );
        std::sort( result.begin(), result.end() );
    }
    return result;
};

//read the file, return a new imagefile struct
imagefile readFile (string path)
{
    imagefile newimage;

    float * iarr1;
    size_t w, h;

    if (NULL == (iarr1 = read_png_f32_gray(path.c_str(), &w, &h))) {
        std::cerr << "Unable to load image file " << path << std::endl;

    }

    std:vector<float> imageData(iarr1, iarr1 + w * h);

    newimage.imageData = imageData;
    newimage.w = w;
    newimage.h = h;
    newimage.framename = path;
    newimage.haskeys = 0;
    free(iarr1); /*memcheck*/

    return newimage;
};
int preloadkeys(imagefile &ifile)
{
    float wS = IM_X;
    float hS = IM_Y;

    float zoom1=0, zoom2=0;
    int wS1=0, hS1=0, wS2=0, hS2=0;
    vector<float> ipixels1_zoom, ipixels2_zoom;

    ipixels1_zoom.resize(ifile.w*ifile.h);
    ipixels1_zoom = ifile.imageData;
    wS1 = ifile.w;
    hS1 = ifile.h;
    zoom1 = 1;



    ///// Compute ASIFT keypoints
    // number N of tilts to simulate t = 1, \sqrt{2}, (\sqrt{2})^2, ..., {\sqrt{2}}^(N-1)
    int num_of_tilts1 = 7;
    int num_of_tilts2 = 7;

    int verb = 0;
    // Define the SIFT parameters
    siftPar siftparameters;
    default_sift_parameters(siftparameters);

    //map the files if it hasnt already. Saving cpu time.


        vector< vector< keypointslist > > tempkeys;
        compute_asift_keypoints(ipixels1_zoom, wS1, hS1, num_of_tilts1, verb, tempkeys, siftparameters);
    ifile.keys = tempkeys;
    ifile.haskeys = 1;
    return 1;
};
//compare two images.
int compImages (imagefile &f1, imagefile &f2)
{
    cout << "Comparing " << f1.framename << " to " << f2.framename << endl;
    ///// Resize the images to area wS*hW in remaining the apsect-ratio
    ///// Resize if the resize flag is not set or if the flag is set unequal to 0
    float wS = IM_X;
    float hS = IM_Y;

    float zoom1=0, zoom2=0;
    int wS1=0, hS1=0, wS2=0, hS2=0;
    vector<float> ipixels1_zoom, ipixels2_zoom;

    ipixels1_zoom.resize(f1.w*f1.h);
    ipixels1_zoom = f1.imageData;
    wS1 = f1.w;
    hS1 = f1.h;
    zoom1 = 1;

    ipixels2_zoom.resize(f2.w*f2.h);
    ipixels2_zoom = f2.imageData;
    wS2 = f2.w;
    hS2 = f2.h;
    zoom2 = 1;

    ///// Compute ASIFT keypoints
    // number N of tilts to simulate t = 1, \sqrt{2}, (\sqrt{2})^2, ..., {\sqrt{2}}^(N-1)
    int num_of_tilts1 = 7;
    int num_of_tilts2 = 7;

    int verb = 0;
    // Define the SIFT parameters
    siftPar siftparameters;
    default_sift_parameters(siftparameters);

    //map the files if it hasnt already. Saving cpu time.
    if(f1.haskeys == 0) {
        cout << "Generating keys for f1." << endl;
        vector< vector< keypointslist > > tempkeys;
        compute_asift_keypoints(ipixels1_zoom, wS1, hS1, num_of_tilts1, verb, tempkeys, siftparameters);
        f1.keys = tempkeys;
        f1.haskeys = 1;
    }
    if(f2.haskeys == 0) {
        vector<vector<keypointslist> > tempkeys;
        compute_asift_keypoints(ipixels2_zoom, wS2, hS2, num_of_tilts2, verb, tempkeys, siftparameters);
        f2.keys = tempkeys;
        f2.haskeys = 1;
    }
    //tend = time(0);
    //  cout << "Keypoints computation accomplished in " << difftime(tend, tstart) << " seconds." << endl;

    //// Match ASIFT keypoints
    int diditmatch;
    matchingslist matchings;
    //cout << "Matching the keypoints..." << endl;
    //tstart = time(0);
    diditmatch = compute_asift_matches(num_of_tilts1, num_of_tilts2, wS1, hS1, wS2,
                                          hS2, verb, f1.keys, f2.keys, matchings, siftparameters);
    //tend = time(0);
    return diditmatch;
}

int main(int argc, char **argv)
{
    time_t tstart, tend;
    tstart = time(0);


    if (((argc != 3))) {
        std::cerr << " Modified. arg1: input dir, arg2: output dir" << std::endl;

        return 1;
    }
    string inpath = argv[1];
    string outpath = argv[2];
	vector<string> indir;
	indir = read_directory(argv[1]);
	if (indir.size() < 1)
    {
        std:cerr << "Input directory has no files. exiting." <<std::endl;
        return 1;
    }

    int currentimage1 = 0;
    int currentimage2 = 1;
    imagefile allimages[indir.size()];
    #pragma omp parallel for
    for (int preint=0;preint<indir.size();++preint)
    {
        imagefile currentimage;
        currentimage = readFile(inpath + indir[preint]);
        preloadkeys(currentimage);
        allimages[preint] = currentimage;

    }

    imagefile curfile1;
    imagefile curfile2;


    //cout << "got here" << endl;
    //curfile1 = readFile(inpath + indir[currentimage1]);
    curfile1= allimages[currentimage1];

//    curfile2 = readFile(inpath + indir[currentimage2]);
    curfile2 = allimages[currentimage2];
    //cout << "got here 2" << endl;

    string fullinpath = inpath + indir[currentimage1];
    string fulloutpath = outpath + indir[currentimage1];
    copyFile(fullinpath.c_str(),fulloutpath.c_str());

    int result = 0;
    for (int it = 0;it<indir.size();++it)
    {

        result = compImages(curfile1,curfile2);
        if (result == 2)
        {
            //images match, continue loop
            currentimage2++;
            if (currentimage2 == indir.size()) break;
            curfile2 = allimages[currentimage2];//readFile(inpath + indir[currentimage2]);
            continue;
        } else {
            //copy new file to outdir.
            string fullinpath = inpath + indir[currentimage2];
            string fulloutpath = outpath + indir[currentimage2];
            copyFile(fullinpath.c_str(),fulloutpath.c_str());

            currentimage1 = currentimage2;
            curfile1 = curfile2;
            currentimage2++;
            if (currentimage2 == indir.size()) break;
            curfile2 = allimages[currentimage2];//readFile(inpath + indir[currentimage2]);


        }

    }

    tend = time(0);
    cout << "Done in: " << difftime(tend, tstart) << " seconds." << endl;
    return 0;
}

