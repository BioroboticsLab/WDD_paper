"""
Clustering and RANSAC for WDD2017 paper
Fernando Wario
fernando.wario@fu-berlin.de
July 2017

Fit model to data using the RANSAC algorithm
Given:
    [options]
    -p --plot: Output format.
    -v --verbose: Verbose mode on.
    
    [arguments]
    file: Path to input file with decoder format.
    cam: Camera to analyze, 1 or 0.
    max_d: Euclidean distance threshold for clustering process.
    n: Minimum number of data values required to compute an angle during RANSAC process.
    t: a threshold value for determining when a data point fits a model during RANSAC process.
    
Return:
    outputFile: Results from the analysis are writen in outputFile with the corresponding format.
"""

import click
import csv
import cv2
import datetime
import glob
import math
import numpy as np
import os
import sys
import scipy.cluster

sys.path.append('functions/')
from wdd_clustering_functions import data_format, generate_clusters, clean_clusters

#Radian
radi = 180/np.pi


@click.command()
@click.option('-p','--plot', is_flag = True, help = 'If defined saves results in plotting format, otherwise saves results in debugging format.')
@click.option('-v','--verbose', is_flag = True, help = 'If defined verbose mode is activated.')
@click.argument('file', type=click.Path(exists=True))
@click.argument('cam', default = '1')
@click.argument('max_d', default = 15)
@click.argument('n', default = 3)
@click.argument('t', default = np.pi/5)
    
              
def Clustering(plot, verbose, file, cam, max_d, n, t):

    #Decoder data is loaded
    A={}
    C={}
    CClusters={}
    #Extract input file's name 
    split_path = file.split("\\")    
    
    if (verbose):        
        click.echo('Current configuration: ')
        click.echo('Input file {0}'.format(split_path[-1]))
        click.echo('cam {0}'.format(cam))
        click.echo('max_d {0}'.format(max_d))
        click.echo('n {0}'.format(n))
        click.echo('t {0}'.format(t))
        if (plot):
            click.echo('Output format: plotting')
        else:
            click.echo('Output format: debugging')
    
    #DECODER DATA
    #Extract data for camera 1, remember to set plot parameter to 1 if plotting on a map
    A = data_format(file, cam, plot)
    if (verbose):
        click.echo('Done loading data to A, {0} WRuns found'.format(len(A)))
    #Generates clusters
    C = generate_clusters(A, max_d)
    if (verbose):
        click.echo('Done with C {0} clusters found'.format(len(C)))
    #clean clusters after removing outlayers and short dances
    CClusters = clean_clusters(A, C, n, t)
    if (verbose):
        click.echo('Done with CClusters {0} clusters remained'.format(len(CClusters)))
    
    #Save data in csv file
    #datetime is used as reference to name the file
    dt = datetime.datetime.now()

    if (plot):        
        outputFile = 'Data/Filtering/' + split_path[-1][:-4] + '_cam' + cam + '_plot_' + str(dt.strftime('%Y%m%d_%H%M')) + '.csv'
        #All GTRuns
        if (os.path.exists(outputFile)):
            os.remove(outputFile)

        with open(outputFile, 'at', newline='') as out_file:
            writer = csv.writer(out_file, delimiter=',', quotechar=' ', quoting=csv.QUOTE_MINIMAL)
            #writer.writerow(['clusterID, avg_length, avg_angle, HH, mm, N_WRuns'])
            for clusterID in sorted(CClusters.keys()):
                # clusterID, avg_length, avg_angle, HH, mm, N_WRuns
                writer.writerow([clusterID, CClusters[clusterID][1], CClusters[clusterID][0], CClusters[clusterID][2], CClusters[clusterID][3], len(CClusters[clusterID][4])])
 
        click.echo('Results saved in ' + outputFile)
        
    else:
        outputFile = 'Data/Filtering/' + split_path[-1][:-4] + '_cam' + cam + '_' + str(dt.strftime('%Y%m%d_%H%M')) + '.csv'
        #All GTRuns
        if (os.path.exists(outputFile)):
            os.remove(outputFile)

        with open(outputFile, 'at', newline='') as out_file:
            #writer = csv.writer(out_file, delimiter=',', quotechar='|', quoting=csv.QUOTE_NONE)
            writer = csv.writer(out_file, delimiter=',', quotechar=' ', quoting=csv.QUOTE_MINIMAL)
            writer.writerow(['clusterID, avg_angle, avg_length, [WRuns keys]'])
            for clusterID in sorted(CClusters.keys()):
                #clean temp toWrite
                toWrite = []
                #clusterID, avg_angle, avg_length
                toWrite = [clusterID, CClusters[clusterID][0], CClusters[clusterID][1]]
                #keys to WRuns are added
                [toWrite.append(i) for i in CClusters[clusterID][4]]
                # clusterID, avg_angle, avg_length, [WRuns keys]
                writer.writerow(toWrite)
        click.echo('Results saved in ' + outputFile)   

if __name__ == "__main__":
    Clustering()