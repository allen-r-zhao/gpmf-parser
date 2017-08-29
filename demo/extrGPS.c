/*! @file extrGPS.c
*
*  @brief Extracts GPS metadata from a GoPro Hero5 Black (untested with Karma); essentially just made minor changes from gpmfdemo.c
*
*  @version 1.0.0
*
*  (C) Copyright 2017 GoPro Inc (http://gopro.com/).
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../GPMF_parser.h"
#include "GPMF_mp4reader.h"


extern void PrintGPMF(GPMF_stream *ms);

int main(int argc, char *argv[])
{	
    printf("bp");
	int32_t ret = GPMF_OK;
	GPMF_stream metadata_stream, *ms = &metadata_stream;
	float metadatalength;
	uint32_t *payload = NULL; //buffer to store GPMF samples from the MP4.
    // get file return data
	if (argc != 2)
	{
		printf("usage: %s <file_with_GPMF>\n", argv[0]);
		return -1;
	}


	metadatalength = OpenGPMFSource(argv[1]);
    printf("%f", metadatalength);	
	if (metadatalength > 0.0)
	{
		// constructs output file
		char *bname = basename(strdup(argv[1]));
        char *dname = dirname(strdup(argv[1]));
        
		bname[strlen(bname)-3] = '\0';
		char str[4 + strlen(dname) + strlen(bname) + 3];  // 'vids/' = 4 letters, name of directory string = ? letters, name of left/right vid file = ? letters,  csv = 3 letters
		strcat(strcat(strcat(strcpy(str,"../../../vids/"), dname), bname), "csv");  // this will look like: "output/your_filename_here.csv"
		
        printf("%s", str);

        FILE *fp = fopen( str, "w" );
		fprintf(fp, "Timestamp (yymmddhhmmss.ss), Latitude (deg), Longitude (deg), GPS Fix\n");
		
		uint32_t index, payloads = GetNumberGPMFPayloads();
		printf("found %.2fs of metadata, from %d payloads, within %s\n", metadatalength, payloads, argv[1]);


		for (index = 0; index < payloads; index++)
		{
			
			int gpsLost = 0;   // flag that is true only when GPS fix is lost 
			uint32_t payloadsize = GetGPMFPayloadSize(index);
			float in = 0.0, out = 0.0; //times
			payload = GetGPMFPayload(payload, index);
			if (payload == NULL)
				goto cleanup;

			ret = GetGPMFPayloadTime(index, &in, &out);
			if (ret != GPMF_OK)
				goto cleanup;


			//printf("MP4 Payload time %.3f to %.3f seconds\n", in, out);

			ret = GPMF_Init(ms, payload, payloadsize);
			if (ret != GPMF_OK)
				goto cleanup;

			
			// Find all the available Streams and the data carrying FourCC
			while (GPMF_OK == GPMF_FindNext(ms, GPMF_KEY_STREAM, GPMF_RECURSE_LEVELS))
			{
				if (GPMF_OK == GPMF_SeekToSamples(ms)) //find the last FOURCC within the stream
				{
					uint32_t key = GPMF_Key(ms);
					GPMF_SampleType type = GPMF_Type(ms);
					uint32_t elements = GPMF_ElementsInStruct(ms);
					uint32_t samples = GPMF_Repeat(ms);

					if (samples && type == GPMF_TYPE_COMPLEX)
					{
						float rate = GetGPMFSampleRateAndTimes(ms, 0.0, index, &in, &out);
						GPMF_stream find_stream;
						GPMF_CopyState(ms, &find_stream);

							if (GPMF_OK == GPMF_FindPrev(&find_stream, GPMF_KEY_TYPE, GPMF_CURRENT_LEVEL))
							{
								char tmp[64];
								char *data = (char *)GPMF_RawData(&find_stream);
								int size = GPMF_RawDataSize(&find_stream);

								if (size < sizeof(tmp))
								{
									memcpy(tmp, data, size);
									tmp[size] = 0;
								}
							}
					}
				}
			}
			GPMF_ResetState(ms);

#if 1		// Find GPS fix value - 0 (no fix), 2 (2D fix), 3 (3D fix)
			if (GPMF_OK == GPMF_FindNext(ms, STR2FOURCC("GPSF"), GPMF_RECURSE_LEVELS) )   //GoPro GPS Timestamps
			{
				uint32_t samples = GPMF_Repeat(ms);
				uint32_t elements = GPMF_ElementsInStruct(ms);
				uint32_t buffersize = samples * elements * sizeof(int);
				
				GPMF_stream find_stream;
				int *tmpbuffer = malloc(buffersize);
				
				if (tmpbuffer && samples)
				{
				  
					GPMF_FormattedData(ms, tmpbuffer, buffersize, 0, samples); // Output data in LittleEnd, but no scale

					if (*tmpbuffer == 0)
					{
					 gpsLost = 1;
					 //printf("%.3f, GPS FIX LOST\n", out);
					} else 
					{
					 gpsLost = 0; 
					 //printf("%.3f, GPS REACQUIRED\n", out);
					}
					
					free(tmpbuffer);
				}
			}
			GPMF_ResetState(ms);
			
#endif 	
			char time[16];  // variable that stores the timestamp for this sample (since timestamps are collected at 1Hz and each sample is 1s)
#if 1		// Find GPS Timestamps and return values. 
			if (GPMF_OK == GPMF_FindNext(ms, STR2FOURCC("GPSU"), GPMF_RECURSE_LEVELS))   //GoPro GPS Timestamps
			{
				uint32_t samples = GPMF_Repeat(ms);
				uint32_t elements = GPMF_ElementsInStruct(ms);
				uint32_t buffersize = samples * elements * sizeof(char);
				
				GPMF_stream find_stream;
				char *tmpbuffer = malloc(buffersize);
				
				if (tmpbuffer && samples)
				{
				  
					GPMF_FormattedData(ms, tmpbuffer, buffersize, 0, samples); // Output data in LittleEnd, but no scale

					tmpbuffer[16] = '\0';
					strcpy(time, tmpbuffer);					
					free(tmpbuffer);
				}
			}
			GPMF_ResetState(ms);
#endif 
			int fix = 0;
#if 1		// Find GPS Fix and return values. 
			if (GPMF_OK == GPMF_FindNext(ms, STR2FOURCC("GPSF"), GPMF_RECURSE_LEVELS))   //GoPro GPS Timestamps
			{
				uint32_t samples = GPMF_Repeat(ms);
				uint32_t elements = GPMF_ElementsInStruct(ms);
				uint32_t buffersize = samples * elements * sizeof(int32_t);
				
				GPMF_stream find_stream;
				int *tmpbuffer = malloc(buffersize);
				
				if (tmpbuffer && samples)
				{
				  
					GPMF_FormattedData(ms, tmpbuffer, buffersize, 0, samples); // Output data in LittleEnd, but no scale
					fix =  *tmpbuffer;					
					free(tmpbuffer);
				}
			}
			GPMF_ResetState(ms);
#endif 			
#if 1		// Find GPS values and return scaled floats. 
			if (GPMF_OK == GPMF_FindNext(ms, STR2FOURCC("GPS5"), GPMF_RECURSE_LEVELS))   //GoPro Hero5 GPS
			{
				uint32_t samples = GPMF_Repeat(ms);
				uint32_t elements = GPMF_ElementsInStruct(ms);
				uint32_t buffersize = samples * elements * sizeof(float);
				GPMF_stream find_stream;
				float *ptr, *tmpbuffer = malloc(buffersize);
				char units[10][6] = { "" };
				uint32_t unit_samples = 1;
				
				if (tmpbuffer && samples)
				{
					uint32_t i, j;
					
					//Search for any units to display
					GPMF_CopyState(ms, &find_stream);
					if (GPMF_OK == GPMF_FindPrev(&find_stream, GPMF_KEY_SI_UNITS, GPMF_CURRENT_LEVEL) ||
						GPMF_OK == GPMF_FindPrev(&find_stream, GPMF_KEY_UNITS, GPMF_CURRENT_LEVEL))
					{
						char *data = (char *)GPMF_RawData(&find_stream);
						int ssize = GPMF_StructSize(&find_stream);
						unit_samples = GPMF_Repeat(&find_stream);

						for (i = 0; i < unit_samples; i++)
						{						
							memcpy(units[i], data, ssize);
							units[i][ssize] = 0;
							data += ssize;
						}
					}

					GPMF_ScaledData(ms, tmpbuffer, buffersize, 0, samples, GPMF_TYPE_FLOAT);  //Output scaled data as floats

					ptr = tmpbuffer;
					if (gpsLost == 0)
					{ 
					  for (i = 0; i < samples; i++)
					  {
						  fprintf(fp, "%s,%.10f,%.10f, %d\n", time, *ptr, *(ptr+1), fix);
					  }
					  free(tmpbuffer);
					} else {
					 fprintf(fp, "%s, GPS_FIX_LOST, GPS_FIX_LOST\n", time); 
					}
				}
			}
			GPMF_ResetState(ms);
			
#endif 
	
		}

	fclose(fp);
	cleanup:
		if (payload) FreeGPMFPayload(payload); payload = NULL;
		CloseGPMFSource();
	}

	return ret;
}
