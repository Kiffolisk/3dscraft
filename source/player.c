#include <3ds.h>

#include "text.h"
#include "player.h"
#include "world.h"
#include "gs.h"

void initPlayer(player_s* p)
{
	if(!p)return;

	initCamera(&p->camera);
	p->camera.position=vect3Df(0.0f, CHUNK_HEIGHT*CLUSTER_SIZE/2+CLUSTER_SIZE, 0.0f);
	loadIdentity44((float*)p->camera.orientation);
	rotateMatrixZ((float*)p->camera.orientation, M_PI/2, false);
	rotateMatrixY((float*)p->camera.orientation, M_PI, false);
	p->velocity=vect3Df(0.0f, 0.0f, 0.0f);
}

void controlsPlayer(player_s* p)
{
	if(!p)return;

	const u32 PAD=hidKeysHeld();
	circlePosition cstick;
	hidCstickRead(&cstick);

	vect3Df_s vx=vnormf(getMatrixColumn((float*)p->camera.orientation, 0));
	vect3Df_s vy=vnormf(getMatrixColumn((float*)p->camera.orientation, 1));
	vect3Df_s vz=vnormf(getMatrixColumn((float*)p->camera.orientation, 2));

	if(PAD&KEY_UP)p->velocity=vaddf(p->velocity, vmulf(vz, -0.2f));
	if(PAD&KEY_DOWN)p->velocity=vaddf(p->velocity, vmulf(vz, 0.2f));
	if(PAD&KEY_RIGHT)p->velocity=vaddf(p->velocity, vmulf(vy, -0.2f));
	if(PAD&KEY_LEFT)p->velocity=vaddf(p->velocity, vmulf(vy, 0.2f));
	if(PAD&KEY_R)p->velocity=vaddf(p->velocity, vmulf(vx, -0.2f));
	if(PAD&KEY_L)p->velocity=vaddf(p->velocity, vmulf(vx, 0.2f));

	// if(PAD&KEY_X)rotateMatrixX((float*)p->camera.orientation, 0.1f, false);
	// if(PAD&KEY_B)rotateMatrixX((float*)p->camera.orientation, -0.1f, false);
	// if(PAD&KEY_A)rotateMatrixY((float*)p->camera.orientation, 0.1f, false);
	// if(PAD&KEY_Y)rotateMatrixY((float*)p->camera.orientation, -0.1f, false);

	rotateMatrixY((float*)p->camera.orientation, (cstick.dx*0.2f)/0x9c, false);
}

//TODO : cleanup ?
vect3Di_s performRayMarch(world_s* w, vect3Df_s localBlockPosf, vect3Df_s localBlockPosf2, vect3Df_s* out)
{
	vect3Di_s cur=vf2i(localBlockPosf);
	vect3Df_s u=vnormf(vsubf(localBlockPosf2, localBlockPosf));
	const float d=vdistf(localBlockPosf2,localBlockPosf);

	const int stepX=(localBlockPosf2.x>localBlockPosf.x)?1:-1;
	const int stepY=(localBlockPosf2.y>localBlockPosf.y)?1:-1;
	const int stepZ=(localBlockPosf2.z>localBlockPosf.z)?1:-1;

	const float tDeltaX=fabs(1.0f/u.x); // w/u.x
	const float tDeltaY=fabs(1.0f/u.y); // h/u.y
	const float tDeltaZ=fabs(1.0f/u.z); // z/u.z

	float tMaxX, tMaxY, tMaxZ;
	if(fabs(u.x)<0.001f)tMaxX=d;
	else tMaxX=fabs((localBlockPosf.x-floorf(localBlockPosf.x)+((localBlockPosf2.x>localBlockPosf.x)?-1.0f:0.0f))/u.x);
	if(fabs(u.y)<0.001f)tMaxY=d;
	else tMaxY=fabs((localBlockPosf.y-floorf(localBlockPosf.y)+((localBlockPosf2.y>localBlockPosf.y)?-1.0f:0.0f))/u.y);
	if(fabs(u.z)<0.001f)tMaxZ=d;
	else tMaxZ=fabs((localBlockPosf.z-floorf(localBlockPosf.z)+((localBlockPosf2.z>localBlockPosf.z)?-1.0f:0.0f))/u.z);

	u8 dir=0;
	if(out)*out=localBlockPosf2;
	do{
		if(tMaxX>=d && tMaxY>=d && tMaxZ>=d)return cur; //finished without colliding with world
		if(tMaxX < tMaxY)
		{
			if(tMaxX < tMaxZ)
			{
				dir=0;
				cur.x+=stepX;
				tMaxX+=tDeltaX;
			}else{
				dir=2;
				cur.z+=stepZ;
				tMaxZ+=tDeltaZ;
			}   
		}else{
			if(tMaxY < tMaxZ) {
				dir=1;
				cur.y+=stepY;
				tMaxY+=tDeltaY;
			}else{
				dir=2;
				cur.z+=stepZ;
				tMaxZ+=tDeltaZ;
			}
		}
	}while(getWorldBlock(w, cur)==0);

	if(out)
	{
		switch(dir)
		{
			case 0:
				{
					float targetX=(cur.x)*1.0f;
					if(stepX<0)targetX+=1.0f;
					float r=(targetX-localBlockPosf.x)/u.x;
					targetX-=0.1f*stepX; //margin
					*out=vaddf(localBlockPosf, vmulf(u,r)); out->x=targetX;
				}
				break;
			case 1:
				{
					float targetY=(cur.y)*1.0f;
					if(stepY<0)targetY+=1.0f;
					float r=(targetY-localBlockPosf.y)/u.y;
					targetY-=0.1f*stepY; //margin
					*out=vaddf(localBlockPosf, vmulf(u,r)); out->y=targetY;
				}
				break;
			case 2:
				{
					float targetZ=(cur.z)*1.0f;
					if(stepZ<0)targetZ+=1.0f;
					float r=(targetZ-localBlockPosf.z)/u.z;
					targetZ-=0.1f*stepZ; //margin
					*out=vaddf(localBlockPosf, vmulf(u,r)); out->z=targetZ;
				}
				break;
		}
	}

	return cur;
}

extern u32 debugValue[];

void updatePlayer(player_s* p, world_s* w)
{
	if(!p)return;

	//gravity goes here
	//collisions go here
	vect3Df_s out;
	if(vmagf(p->velocity)>0.0f)
	{
		performRayMarch(w, p->camera.position, vaddf(p->camera.position, p->velocity), &out);
		p->camera.position=out;
		// print("%f %f %f\n",p->camera.position.x,p->camera.position.y,p->camera.position.z);
	}

	// p->camera.position=vaddf(p->camera.position, p->velocity);

	p->velocity=vect3Df(0.0f, 0.0f, 0.0f);

	updateCamera(&p->camera);
}

void setCameraPlayer(player_s* p)
{
	if(!p)return;

	setCamera(&p->camera);
}
