#include <stdio.h>
//#include <stdlib.h>
#include <math.h>
#include <malloc.h>
//#include <time.h>
#include <iostream>

#include "wg2D.h"
#include "commonConstants.h"
//#include "furtherMathFunctions.h"
//#include "statistics.h"
//#include "eispack.h"
//#include "gammaFunction.h"

#include "weatherGenerator.h"
#include "wgClimate.h"


void weatherGenerator2D::initializeOutputData(int* nrDays)
{
    int length = 365*parametersModel.yearOfSimulation;
    for (int i=1; i<= parametersModel.yearOfSimulation;i++)
    {
        if (isLeapYear(i)) length++;
    }
    *nrDays = length;
    outputWeatherData = (ToutputWeatherData*)calloc(nrStations, sizeof(ToutputWeatherData));
    for (int iStation=0;iStation<nrStations;iStation++)
    {
        outputWeatherData[iStation].yearSimulated = (int*)calloc(length, sizeof(int));
        outputWeatherData[iStation].monthSimulated = (int*)calloc(length, sizeof(int));
        outputWeatherData[iStation].daySimulated = (int*)calloc(length, sizeof(int));
        outputWeatherData[iStation].doySimulated = (int*)calloc(length, sizeof(int));
        //if (isTempWG2D)
        //{
            outputWeatherData[iStation].maxT = (double*)calloc(length, sizeof(double));
            outputWeatherData[iStation].minT = (double*)calloc(length, sizeof(double));
        //}
        //if (isPrecWG2D)
            outputWeatherData[iStation].precipitation = (double*)calloc(length, sizeof(double));
    }
    // = (double*)calloc(lengthOfRandomSeries, sizeof(double));
}

void weatherGenerator2D::getWeatherGeneratorOutput()
{
    int counter;
    int counterSeason[4];
    int day,month;
    int nrDays = NODATA;
    weatherGenerator2D::initializeOutputData(&nrDays);
    Crit3DDate inputFirstDate;
    TweatherGenClimate weatherGenClimate;
    QString outputFileName;


    float *inputTMin = nullptr;
    float *inputTMax = nullptr;
    float *inputPrec = nullptr;
    float precThreshold = parametersModel.precipitationThreshold;
    float minPrecData = NODATA;
    bool writeOutput = true;
    inputTMin = (float*)calloc(nrDays, sizeof(float));
    inputTMax = (float*)calloc(nrDays, sizeof(float));
    inputPrec = (float*)calloc(nrDays, sizeof(float));
    inputFirstDate.day = 1;
    inputFirstDate.month = 1;
    inputFirstDate.year = 1;
    float ** monthlySimulatedAveragePrecipitation = nullptr;
    float ** monthlyClimateAveragePrecipitation = nullptr;
    float ** monthlySimulatedAveragePrecipitationInternalFunction = nullptr;
    float ** monthlyClimateAveragePrecipitationInternalFunction = nullptr;
    float** meanAmountsPrecGenerated = nullptr;
    float** cumulatedOccurrencePrecGenerated = nullptr;
    monthlySimulatedAveragePrecipitation = (float**)calloc(nrStations, sizeof(float*));
    monthlyClimateAveragePrecipitation = (float**)calloc(nrStations, sizeof(float*));
    monthlySimulatedAveragePrecipitationInternalFunction = (float**)calloc(nrStations, sizeof(float*));
    monthlyClimateAveragePrecipitationInternalFunction = (float**)calloc(nrStations, sizeof(float*));
    meanAmountsPrecGenerated = (float**)calloc(nrStations, sizeof(float*));
    cumulatedOccurrencePrecGenerated = (float**)calloc(nrStations, sizeof(float*));
    for (int iStation=0;iStation<nrStations;iStation++)
    {
        meanAmountsPrecGenerated[iStation] = (float*)calloc(12, sizeof(float));
        cumulatedOccurrencePrecGenerated[iStation] = (float*)calloc(12, sizeof(float));
        monthlySimulatedAveragePrecipitation[iStation] = (float*)calloc(12, sizeof(float));
        monthlyClimateAveragePrecipitation[iStation] = (float*)calloc(12, sizeof(float));
        monthlySimulatedAveragePrecipitationInternalFunction[iStation] = (float*)calloc(12, sizeof(float));
        monthlyClimateAveragePrecipitationInternalFunction[iStation] = (float*)calloc(12, sizeof(float));
        for (int j=0;j<12;j++)
        {
            meanAmountsPrecGenerated[iStation][j] = 0;
            cumulatedOccurrencePrecGenerated[iStation][j] = 0;
            monthlySimulatedAveragePrecipitation[iStation][j] = NODATA;
            monthlyClimateAveragePrecipitation[iStation][j] = NODATA;
            monthlySimulatedAveragePrecipitationInternalFunction[iStation][j] = 0;
            monthlyClimateAveragePrecipitationInternalFunction[iStation][j] = 0;
        }
    }
    for (int iStation=0;iStation<nrStations;iStation++)
    {
        for (int iDay=0;iDay<nrData;iDay++)
        {
            if (obsDataD[iStation][iDay].prec > parametersModel.precipitationThreshold)
            {
                meanAmountsPrecGenerated[iStation][obsPrecDataD[iStation][iDay].date.month-1] += obsDataD[iStation][iDay].prec;
                ++cumulatedOccurrencePrecGenerated[iStation][obsPrecDataD[iStation][iDay].date.month-1];
            }
        }
        for (int jMonth=0;jMonth<12;jMonth++)
        {
            meanAmountsPrecGenerated[iStation][jMonth] /= cumulatedOccurrencePrecGenerated[iStation][jMonth];
            //printf("%d  %f %f\n",jMonth,meanAmountsPrecGenerated[iStation][jMonth],cumulatedOccurrencePrecGenerated[iStation][jMonth]);
        }
        //pressEnterToContinue();
    }

    for (int iStation=0;iStation<nrStations;iStation++)
    {
        outputFileName = "wgSimulation_station_" + QString::number(iStation) + ".txt";
        counter = 0;
        counterSeason[3] = counterSeason[2] = counterSeason[1] = counterSeason[0] = 0;
        for (int iYear=1;iYear<=parametersModel.yearOfSimulation;iYear++)
        {
            //counterSeason[3] = counterSeason[2] = counterSeason[1] = counterSeason[0] = 0;
            for (int iDoy=0; iDoy<365; iDoy++)
            {
                outputWeatherData[iStation].yearSimulated[counter] = iYear;
                outputWeatherData[iStation].doySimulated[counter] = iDoy+1;
                weatherGenerator2D::dateFromDoy(outputWeatherData[iStation].doySimulated[counter],1,&day,&month);
                outputWeatherData[iStation].daySimulated[counter] = day;
                outputWeatherData[iStation].monthSimulated[counter] = month;
                if (isTempWG2D)
                {
                    outputWeatherData[iStation].maxT[counter] = maxTGenerated[counter][iStation];
                    outputWeatherData[iStation].minT[counter] = minTGenerated[counter][iStation];
                    //printf("%.2f %.2f %.0f\n",outputWeatherData[0].maxT[counter],outputWeatherData[0].minT[counter],occurrencePrecGenerated[counter][0]);
                }
                else
                {
                    outputWeatherData[iStation].maxT[counter] = NODATA;
                    outputWeatherData[iStation].minT[counter] = NODATA;

                }

                if (isPrecWG2D)
                {

                    if (parametersModel.distributionPrecipitation == 3)
                    {
                        outputWeatherData[iStation].precipitation[counter] = precGenerated[iStation][counter];
                    }
                    else {outputWeatherData[iStation].precipitation[counter] = amountsPrecGenerated[counter][iStation];}
                }
                else
                {
                    outputWeatherData[iStation].precipitation[counter] = occurrencePrecGenerated[counter][iStation] + 0.1;
                }
                counter++;
            }
        }

        /*
        for(int i=0;i<parametersModel.yearOfSimulation*365;i++)
        {
            //printf("%d %d %.1f %.1f %.1f\n",outputWeatherData[iStation].daySimulated[i],outputWeatherData[iStation].monthSimulated[i],outputWeatherData[iStation].minT[i],outputWeatherData[iStation].maxT[i],outputWeatherData[iStation].precipitation[i]);
            //printf("%d %d %.1f %.1f %.1f\n",outputWeatherData[iStation].daySimulated[i],outputWeatherData[iStation].monthSimulated[i],outputWeatherData[iStation].minT[i],outputWeatherData[iStation].maxT[i],outputWeatherData[iStation].precipitation[i]);
            //printf("%d %d %.1f\n",outputWeatherData[iStation].daySimulated[i],outputWeatherData[iStation].monthSimulated[i],outputWeatherData[iStation].precipitation[i]);

        }*/
        //pressEnterToContinue();
        counter = 0;
        for (int i=0;i<nrDays;i++)
        {
            inputTMin[i]= (float)(outputWeatherData[iStation].minT[counter]);
            inputTMax[i]= (float)(outputWeatherData[iStation].maxT[counter]);
            inputPrec[i]= (float)(outputWeatherData[iStation].precipitation[counter]);            
            if (isLeapYear(outputWeatherData[iStation].yearSimulated[counter]) && outputWeatherData[iStation].monthSimulated[counter] == 2 && outputWeatherData[iStation].daySimulated[counter] == 28)
            {
                ++i;
                inputTMin[i]= (float)(outputWeatherData[iStation].minT[counter]);
                inputTMax[i]= (float)(outputWeatherData[iStation].maxT[counter]);
                inputPrec[i]= (float)(outputWeatherData[iStation].precipitation[counter]);

            }
            counter++;
        }
        computeWG2DClimate(nrDays,inputFirstDate,inputTMin,inputTMax,inputPrec,precThreshold,minPrecData,&weatherGenClimate,false,outputFileName,monthlySimulatedAveragePrecipitation[iStation]);
    }

    free(inputTMin);
    free(inputTMax);
    free(inputPrec);

    precThreshold = float(parametersModel.precipitationThreshold);
    minPrecData = NODATA;
    nrDays = nrData;
    inputTMin = (float*)calloc(nrDays, sizeof(float));
    inputTMax = (float*)calloc(nrDays, sizeof(float));
    inputPrec = (float*)calloc(nrDays, sizeof(float));
    // compute climate statistics from observed data
    for (int iStation=0;iStation<nrStations;iStation++)
    {
        outputFileName = "wgClimate_station_" + QString::number(iStation) + ".txt";
        inputFirstDate.day = obsDataD[iStation][0].date.day;
        inputFirstDate.month = obsDataD[iStation][0].date.month;
        inputFirstDate.year = obsDataD[iStation][0].date.year;
        nrDays = nrData;
        for (int i=0;i<nrDays;i++)
        {
            inputTMin[i] = obsDataD[iStation][i].tMin;
            inputTMax[i] = obsDataD[iStation][i].tMax;
            inputPrec[i] = obsDataD[iStation][i].prec;
        }
        computeWG2DClimate(nrDays,inputFirstDate,inputTMin,inputTMax,inputPrec,precThreshold,minPrecData,&weatherGenClimate,writeOutput,outputFileName,monthlyClimateAveragePrecipitation[iStation]);
    }
    free(inputTMin);
    free(inputTMax);
    free(inputPrec);
    weatherGenerator2D::precipitationMonthlyAverage(monthlySimulatedAveragePrecipitationInternalFunction,monthlyClimateAveragePrecipitationInternalFunction);
    for (int iStation=0;iStation<nrStations;iStation++)
    {
        //printf("stazione %d\n",iStation);
        for (int iMonth=0;iMonth<12;iMonth++)
        {
            //printf("%f  %f \n",monthlySimulatedAveragePrecipitation[iStation][iMonth],monthlyClimateAveragePrecipitation[iStation][iMonth]);
            //printf("%f  %f \n",monthlySimulatedAveragePrecipitationInternalFunction[iStation][iMonth],monthlyClimateAveragePrecipitationInternalFunction[iStation][iMonth]);
        }
        //pressEnterToContinue();
    }

    for (int iStation=0;iStation<nrStations;iStation++)
    {
        for (int iDate=0;iDate<parametersModel.yearOfSimulation*365;iDate++)
        {
            int doy,day,month;
            day = month = 0;
            doy = iDate%365 + 1;
            weatherGenerator2D::dateFromDoy(doy,2001,&day,&month);
            if (outputWeatherData[iStation].precipitation[iDate] > parametersModel.precipitationThreshold + EPSILON)
            {
                outputWeatherData[iStation].precipitation[iDate] = MAXVALUE(parametersModel.precipitationThreshold + EPSILON,outputWeatherData[iStation].precipitation[iDate]* monthlyClimateAveragePrecipitationInternalFunction[iStation][month-1] / monthlySimulatedAveragePrecipitationInternalFunction[iStation][month-1]);
            }

        }
    }

    nrDays = 365*parametersModel.yearOfSimulation;
    for (int i=1;i<=parametersModel.yearOfSimulation;i++)
    {
        if (isLeapYear(i)) nrDays++;
    }
    inputTMin = (float*)calloc(nrDays, sizeof(float));
    inputTMax = (float*)calloc(nrDays, sizeof(float));
    inputPrec = (float*)calloc(nrDays, sizeof(float));
    inputFirstDate.day = 1;
    inputFirstDate.month = 1;
    inputFirstDate.year = 1;


    for (int iStation=0;iStation<nrStations;iStation++)
    {
        outputFileName = "wgSimulation_2_station_" + QString::number(iStation) + ".txt";
        counter = 0;
        for (int i=0;i<nrDays;i++)
        {
            inputTMin[i]= (float)(outputWeatherData[iStation].minT[counter]);
            inputTMax[i]= (float)(outputWeatherData[iStation].maxT[counter]);
            //if (isPrecWG2D)
            inputPrec[i]= (float)(outputWeatherData[iStation].precipitation[counter]);
            //else inputPrec[i]= 0;
            if (isLeapYear(outputWeatherData[iStation].yearSimulated[counter]) && outputWeatherData[iStation].monthSimulated[counter] == 2 && outputWeatherData[iStation].daySimulated[counter] == 28)
            {
                ++i;
                inputTMin[i]= (float)(outputWeatherData[iStation].minT[counter]);
                inputTMax[i]= (float)(outputWeatherData[iStation].maxT[counter]);
                inputPrec[i]= (float)(outputWeatherData[iStation].precipitation[counter]);
            }
            counter++;
        }
        computeWG2DClimate(nrDays,inputFirstDate,inputTMin,inputTMax,inputPrec,precThreshold,minPrecData,&weatherGenClimate,writeOutput,outputFileName,monthlySimulatedAveragePrecipitation[iStation]);
    }

    free(inputTMin);
    free(inputTMax);
    free(inputPrec);

    weatherGenerator2D::precipitationCorrelationMatricesSimulation();





}

void weatherGenerator2D::precipitationCorrelationMatricesSimulation()
{
    int counter =0;
    TcorrelationVar amount,occurrence;
    TcorrelationMatrix* correlationMatrixSimulation = nullptr;
    correlationMatrixSimulation = (TcorrelationMatrix*)calloc(12, sizeof(TcorrelationMatrix));
    for (int iMonth=0;iMonth<12;iMonth++)
    {
        correlationMatrixSimulation[iMonth].amount = (double**)calloc(nrStations, sizeof(double*));
        correlationMatrixSimulation[iMonth].occurrence = (double**)calloc(nrStations, sizeof(double*));
        for (int i=0;i<nrStations;i++)
        {
            correlationMatrixSimulation[iMonth].amount[i]= (double*)calloc(nrStations, sizeof(double));
            correlationMatrixSimulation[iMonth].occurrence[i]= (double*)calloc(nrStations, sizeof(double));
            for (int ii=0;ii<nrStations;ii++)
            {
                correlationMatrixSimulation[iMonth].amount[i][ii]= NODATA;
                correlationMatrixSimulation[iMonth].occurrence[i][ii]= NODATA;
            }
        }
    }
    for (int iMonth=0;iMonth<12;iMonth++)
    {
        correlationMatrixSimulation[iMonth].month = iMonth + 1 ; // define the month of the correlation matrix;
        for (int k=0; k<nrStations;k++) // correlation matrix diagonal elements;
        {
            correlationMatrixSimulation[iMonth].amount[k][k] = 1.;
            correlationMatrixSimulation[iMonth].occurrence[k][k]= 1.;
        }

        for (int j=0; j<nrStations-1;j++)
        {
            for (int i=j+1; i<nrStations;i++)
            {
                counter = 0;
                amount.meanValue1=0.;
                amount.meanValue2=0.;
                amount.covariance = amount.variance1 = amount.variance2 = 0.;
                occurrence.meanValue1=0.;
                occurrence.meanValue2=0.;
                occurrence.covariance = occurrence.variance1 = occurrence.variance2 = 0.;

                for (int k=0; k<365*parametersModel.yearOfSimulation;k++) // compute the monthly means
                {
                    int doy,day,month;
                    day = month = 0;
                    doy = k%365 + 1;
                    weatherGenerator2D::dateFromDoy(doy,2001,&day,&month);
                    if (month == (iMonth+1))
                    {
                        if (((outputWeatherData[j].precipitation[k] - NODATA) > EPSILON) && ((outputWeatherData[i].precipitation[k] - NODATA) > EPSILON))
                        {
                            counter++;
                            if (outputWeatherData[j].precipitation[k] > parametersModel.precipitationThreshold)
                            {
                                amount.meanValue1 += outputWeatherData[j].precipitation[k] ;
                                occurrence.meanValue1++ ;
                            }
                            if (outputWeatherData[i].precipitation[k] > parametersModel.precipitationThreshold)
                            {
                                amount.meanValue2 += outputWeatherData[i].precipitation[k];
                                occurrence.meanValue2++ ;
                            }
                        }
                    }
                }
                if (counter != 0)
                {
                    amount.meanValue1 /= counter;
                    occurrence.meanValue1 /= counter;
                }

                if (counter != 0)
                {
                    amount.meanValue2 /= counter;
                    occurrence.meanValue2 /= counter;
                }
                // compute the monthly rho off-diagonal elements
                for (int k=0; k<365*parametersModel.yearOfSimulation;k++)
                {
                    int doy,day,month;
                    day = month = 0;
                    doy = k%365+1;
                    weatherGenerator2D::dateFromDoy(doy,2001,&day,&month);
                    if (month == (iMonth+1))
                    {
                        if ((outputWeatherData[j].precipitation[k] != NODATA) && (outputWeatherData[i].precipitation[k] != NODATA))
                        {
                            double value1,value2;
                            if (outputWeatherData[j].precipitation[k] <= parametersModel.precipitationThreshold) value1 = 0.;
                            else value1 = outputWeatherData[j].precipitation[k];
                            if (outputWeatherData[i].precipitation[k] <= parametersModel.precipitationThreshold) value2 = 0.;
                            else value2 = outputWeatherData[i].precipitation[k];

                            amount.covariance += (value1 - amount.meanValue1)*(value2 - amount.meanValue2);
                            amount.variance1 += (value1 - amount.meanValue1)*(value1 - amount.meanValue1);
                            amount.variance2 += (value2 - amount.meanValue2)*(value2 - amount.meanValue2);

                            if (outputWeatherData[j].precipitation[k] <= parametersModel.precipitationThreshold) value1 = 0.;
                            else value1 = 1.;
                            if (outputWeatherData[i].precipitation[k] <= parametersModel.precipitationThreshold) value2 = 0.;
                            else value2 = 1.;

                            occurrence.covariance += (value1 - occurrence.meanValue1)*(value2 - occurrence.meanValue2);
                            occurrence.variance1 += (value1 - occurrence.meanValue1)*(value1 - occurrence.meanValue1);
                            occurrence.variance2 += (value2 - occurrence.meanValue2)*(value2 - occurrence.meanValue2);
                        }
                    }
                }
                correlationMatrixSimulation[iMonth].amount[j][i]= amount.covariance / sqrt(amount.variance1*amount.variance2);
                correlationMatrixSimulation[iMonth].amount[i][j] = correlationMatrixSimulation[iMonth].amount[j][i];
                correlationMatrixSimulation[iMonth].occurrence[j][i]= occurrence.covariance / sqrt(occurrence.variance1*occurrence.variance2);
                correlationMatrixSimulation[iMonth].occurrence[i][j] = correlationMatrixSimulation[iMonth].occurrence[j][i];
            }
        }

    }
    FILE* fp;
    fp = fopen("correlationMatrices.txt","w");
    for (int iMonth=0;iMonth<12;iMonth++)
    {
        //printf("month %d \n",iMonth+1);
        //printf("observed\n");
        fprintf(fp,"month %d \n",iMonth+1);
        //fprintf(fp,"observed\n");
        for (int i=0;i<nrStations;i++)
        {
            for (int j=0;j<nrStations;j++)
            {
                //printf("%.2f ", correlationMatrix[iMonth].amount[j][i]);
                //fprintf(fp,"%.2f ", correlationMatrix[iMonth].amount[j][i]);
            }
            //printf("\n");
            //fprintf(fp,"\n");
        }

        //printf("simulated\n");
        //fprintf(fp,"simulated\n");
        for (int i=0;i<nrStations;i++)
        {
            for (int j=0;j<nrStations;j++)
            {
                //printf("%.2f ", correlationMatrixSimulation[iMonth].amount[j][i]);
                //fprintf(fp,"%.2f ", correlationMatrixSimulation[iMonth].amount[j][i]);
            }
            //printf("\n");
            //fprintf(fp,"\n");
        }

        //printf("simulated - observed\n");
        fprintf(fp,"simulated - observed\n");
        for (int i=0;i<nrStations;i++)
        {
            for (int j=0;j<nrStations;j++)
            {
                //printf("%.2f ", correlationMatrixSimulation[iMonth].amount[j][i]-correlationMatrix[iMonth].amount[j][i]);
                fprintf(fp,"%.2f ", correlationMatrixSimulation[iMonth].amount[j][i]-correlationMatrix[iMonth].amount[j][i]);
            }
            //printf("\n");
            fprintf(fp,"\n");
        }

        //pressEnterToContinue();
    }
    fclose(fp);
    for (int iMonth=0;iMonth<12;iMonth++)
    {
        for (int i=0;i<nrStations;i++)
        {
            free(correlationMatrixSimulation[iMonth].amount[i]);
            free(correlationMatrixSimulation[iMonth].occurrence[i]);
        }
    }
    free(correlationMatrixSimulation);


}

void weatherGenerator2D::precipitationMonthlyAverage(float** averageSimulation, float** averageClimate)
{
    for (int iStation=0 ; iStation < nrStations ; iStation++)
    {
        int counterMonth[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
        int doy,day,month;
        doy = day = month = 0;

        for (int i=0; i<365*parametersModel.yearOfSimulation; i++)
        {
            doy = i%365+1;
            weatherGenerator2D::dateFromDoy(doy,2001,&day,&month);
            if (outputWeatherData[iStation].precipitation[i] > parametersModel.precipitationThreshold)
            {
                averageSimulation[iStation][month-1] += outputWeatherData[iStation].precipitation[i];
                ++counterMonth[month-1];
            }
        }
        for (int iMonth=0; iMonth<12; iMonth++)
        {
            averageSimulation[iStation][iMonth] /= parametersModel.yearOfSimulation;
        }
    }

    for (int iStation=0 ; iStation < nrStations ; iStation++)
    {
        int counterMonth[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
        int doy,day,month;
        doy = day = month = 0;
        for (int i=0; i<nrData; i++)
        {
            //doy = (i+1)%365;
            //weatherGenerator2D::dateFromDoy(doy,2001,&day,&month);
            month = obsDataD[iStation][i].date.month;
            if (obsDataD[iStation][i].prec > parametersModel.precipitationThreshold) // including that prec != -9999
            {
                averageClimate[iStation][month-1] += obsDataD[iStation][i].prec;
                ++counterMonth[month-1];
            }
        }

        for (int iMonth=0; iMonth<12; iMonth++)
        {
            averageClimate[iStation][iMonth] /= (obsDataD[iStation][nrData-1].date.year - obsDataD[iStation][0].date.year);
            //printf("prova %f  %d",averageClimate[iStation][iMonth],counterMonth[iMonth]);
        }
        //pressEnterToContinue();
    }
}
