/////////////////////////////////////////////////////////////////////////
// Software License Agreement (BSD License)                            //
//                                                                     //
// Copyright (c) 2011                                                  //
// Gerard Benjamin Pierre                                              //
// imdb     : http://www.imdb.com/name/nm0682633/                      //
// linkedin : http://www.linkedin.com/profile/view?id=9048639          //
//                                                                     //
// All rights reserved.                                                //
//                                                                     //
// Redistribution and use in source and binary forms, with or without  //
// modification, are permitted provided that the following conditions  //
// are met:                                                            //
//                                                                     //
//  * Redistributions of source code must retain the above copyright   //
//    notice, this list of conditions and the following disclaimer.    //
//  * Redistributions in binary form must reproduce the above          //
//    copyright notice, this list of conditions and the following      //
//    disclaimer in the documentation and/or other materials provided  //
//    with the distribution.                                           //
//  * Neither the name of the EPFL nor the names of its                //
//    contributors may be used to endorse or promote products derived  //
//    from this software without specific prior written permission.    //
//                                                                     //
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS //
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT   //
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   //
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE      //
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, //
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,//                              
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;    //
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER    //
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT  //
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN   //
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE     //
// POSSIBILITY OF SUCH DAMAGE.                                         //
/////////////////////////////////////////////////////////////////////////

static const char* CLASS = "ImageTool";

static const char* HELP =
        "ImageTool\n\n"

        "Similar to CurveTool, but returning the pixels rather than curves.\n"
        "(Same as the Stat node in Shake).\n";

#include "DDImage/Iop.h"
#include "DDImage/Thread.h"
#include "DDImage/Row.h"
#include "DDImage/Tile.h"
#include "DDImage/Pixel.h"
#include "DDImage/Knobs.h"
#include "DDImage/DDMath.h"
#include "DDImage/NukeWrapper.h"


#include <stdio.h>
#include <vector>
#include <algorithm>

using namespace DD::Image;
using namespace std;

enum {AVG,MED,LOW,HIGH};
const char* const modes[] = {"Average","Median","Low","High",NULL};

class ImageTool : public Iop
{
	Lock _lock;
	volatile bool _firstTime;

	int mode;	
	
	float apix[4];
	float mpix[4];
	float hpix[4];
	float lpix[4];
	
	int fx;
	int fy;
	int fr;
	int ft;

public:

	ImageTool(Node* node) : Iop(node), _firstTime(true)
	{
		mode = AVG;
	}

	~ImageTool()
	{
	}

	void _validate(bool for_real)
	{
		copy_info();
		
		_firstTime = true;
	}

	void _request(int x, int y, int r, int t, ChannelMask channels, int count)
	{
		input0().request(x, y, r, t, channels, count);
	}

	int int_channel(Channel z)
	{
		switch (z)
		{
		case Chan_Red:
			return 0;
		case Chan_Green:
			return 1;
		case Chan_Blue:
			return 2;
		case Chan_Alpha:
			return 3;
		default:
			return -1;
		}
	}

	void engine(int y, int x, int r, ChannelMask channels, Row& out)
	{
		if (aborted())
			return;

		Box box = input0().info();
		fx = box.x();
		fy = box.y();
		fr = box.r();
		ft = box.t();
		int m_w = max(fr-fx,1);
		int m_h = max(ft-fy,1);
		Tile tile(input0(),fx,fy,fr,ft+1,channels);

		{
			Guard guard(_lock);
			if ( _firstTime )
			{
				Interest interest( input0(), fx, fy, fr, ft+1, channels, true );
				interest.unlock();
				
				// do analysis.
				foreach(z, channels)
				{
					int c = int_channel(z);
					if (c == -1)
						continue;
					
					float sum = 0;
					vector < float > Pixels;
					
					for (int dy = fy; dy < ft; dy++) {
						
						progressFraction( dy, m_h );
						Row in0(fx, fr);
						input0().get(fy, fx, fr, channels, in0);
						if ( aborted() )
							return;
						
						for (int dx = fx; dx < fr; dx++) {
							if (mode == AVG)
							{
								sum += tile[z][tile.clampy(dy)][tile.clampx(dx)];
							} 
							else
							{
								Pixels.push_back(tile[z][tile.clampy(dy)][tile.clampx(dx)]);
							}
						}
					}
					
					int Psize = Pixels.size();
					if (mode != AVG)
					{
						if (Psize)
						{
							sort( Pixels.begin(), Pixels.end() );
							hpix[z] = Pixels[Psize-1];
							mpix[z] = Pixels[Psize/2];
							lpix[z] = Pixels[0];
						}
						else
						{
							//we didn't collect any pixels
							//set some generic values
							hpix[z] = 1.0;
							mpix[z] = 0.5;
							lpix[z] = 0.0;
						}
					}
					else
					{
						apix[z] = sum/(m_w*m_h);
					}
				}
				_firstTime = false;
			}
		} // end lock
				
		Row inrow(x, r);
		input0().get(y, x, r, channels, inrow);
		
		foreach (z, channels) {
			
			int c = int_channel(z);
			if (c == -1)
			{
				out.copy(inrow, z, x, r);
				continue;
			}
			
			float pix = (mode != AVG ? ((mode != MED) ? ((mode == LOW) ? lpix[z] : hpix[z]) : mpix[z]) : apix[z]);
			
			float* outptr = out.writable(z) + x;
			
			for (int X = x; X < r; X++)
			{
				*outptr++ = pix;
			}
		}
	}

	virtual void knobs(Knob_Callback f)
	{
		Text_knob(f, "ImageTool","(c) Ben Pierre 2011 "); SetFlags(f, Knob::STARTLINE);
		Enumeration_knob(f, &mode, modes, "mode");
		SetFlags(f, Knob::NO_ANIMATION | Knob::NO_UNDO);
		Tooltip(f, "Return the image by this mode.");
	}

	const char* Class() const { return CLASS; }
	const char* node_help() const { return HELP; }

	static const Op::Description d;

};

static Op* construct(Node* node) { return (new NukeWrapper(new ImageTool(node)))->channels(Mask_RGBA); }
const Op::Description ImageTool::d(CLASS, construct);

// end of ImageTool.cpp
