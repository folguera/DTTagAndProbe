import argparse
import sys
import os
import json

"""
Setup argument parser
"""

parser = argparse.ArgumentParser(description="This program takes an input JSON config and extracts plots from Tag-and-Probe ROOT files. The output consists of a plot with superimposed graphs from multiple TnP files and the according fit canvases.")
parser.add_argument("inputJsonConfig", help="Path to the input JSON config file")
parser.add_argument("-f", "--fast", default=0, action="count", help="Skip fetching and saving the fit canvases for each plot")
parser.add_argument("-v", "--verbosity", default=1, help="Increase or decrease output verbosity")
args = parser.parse_args()

"""
Parse JSON file
"""

with open(args.inputJsonConfig, 'r') as f:
    config = json.loads(f.read())

"""
Go through plots defined in config JSON
"""

from ROOT import * # import this here, otherwise it overwrites the argparse stuff
gROOT.SetBatch(True) # set ROOT to batch mode, this suppresses printing canvases
gROOT.ProcessLine("gErrorIgnoreLevel = 1001;") # suppress stdout pollution of canvas.Print(...)

TH1.AddDirectory(False)
gStyle.SetOptTitle(0)
gStyle.SetOptStat(0)

for keyPlot in config:
    if args.verbosity==1:
        print('Processing plot config: {}'.format(keyPlot))
        print('Config comment: {}'.format(config[keyPlot]['comment']))

    # Get result plots from fit canvases (in dir fit_eff_plots)

    # Get input parameters
    inputFilenames = []
    inputPlotNames = []
    inputFolders = []
    inputLegendEntries = []
    inputLabels = []
 
    for keyInputs in sorted(config[keyPlot]['inputs']):
        inputFilenames.append(config[keyPlot]['inputs'][keyInputs]['filename'])
        inputPlotNames.append(config[keyPlot]['inputs'][keyInputs]['plot'])
        inputFolders.append(config[keyPlot]['inputs'][keyInputs]['folder'])
        inputLegendEntries.append(config[keyPlot]['inputs'][keyInputs]['legendEntry'])
        inputLabels.append(config[keyPlot]['inputs'][keyInputs]['label'])

    # Get input graphs from files
    inputHistos = {}
    for iHisto in range(len(inputFilenames)):
        inputFile = TFile.Open(inputFilenames[iHisto])
        if not inputFile:
            print('[ERROR] File not found: {}'.format(inputFilenames[iHisto]))
            sys.exit()
        inputDir = inputFile.GetDirectory(inputFolders[iHisto])
        inputName = None
        for keys in inputDir.GetListOfKeys():
            if keys.GetName() == inputPlotNames[iHisto] or inputPlotNames[iHisto] == "all" :
                inputName = keys.GetName()

                if inputName.find("=") > 0 or \
                   inputDir.Get(inputName).ClassName() == "TDirectoryFile" or \
                   inputDir.Get(inputName).ClassName() == "TH2F":
                    continue

                if inputPlotNames[iHisto] == "all" :
                    nameTag = inputName
                else :
                    nameTag = "histo"

                if not inputHistos.has_key(nameTag):
                    inputHistos[nameTag] = []
            
                print('Load plot \'{}\': {}'.format(inputFolders[iHisto], inputName))
                print inputDir.Get(inputName).ClassName()

                histo = inputDir.Get(inputName).Clone(str(iHisto))
                inputHistos[nameTag].append(histo)

    # Set line color and marker style for each graph using given maps from config
    colorMap = config[keyPlot]['plot']['colorMap']
    markerMap = config[keyPlot]['plot']['markerMap']
    if args.verbosity==1:
        print('Using colormap: {}'.format(colorMap))
        print('Using markermap: {}'.format(markerMap))
    if len(colorMap)<len(inputFolders):
        print('[ERROR] The defined colormap has not enough entries for the number of defined input folders.')
        sys.exit()
    if len(markerMap)<len(inputFolders):
        print('[ERROR] The defined markermap has not enough entries for the number of defined input folders.')
        sys.exit()

    for histoName,histograms in inputHistos.iteritems():

        for iHisto in range(len(histograms)):

            histograms[iHisto].SetLineColor(colorMap[iHisto])
            histograms[iHisto].SetLineWidth(2)
            histograms[iHisto].SetMarkerStyle(markerMap[iHisto])
            histograms[iHisto].SetMarkerColor(colorMap[iHisto])

        # Setup canvas with all elements
        canvas = TCanvas('canvas', 'canvas', 800, 800)


        pad = TPad('pad', 'pad', 0.01, 0.00, 1.00, 1.00)
 
        pad.SetGrid()
        #pad.SetLogy()
        pad.Draw()
        pad.cd()

        plotX = config[keyPlot]['plot']['x']
        plotY = config[keyPlot]['plot']['y']
        # Generate superimposed graph using TMultiHisto

        if histograms[iHisto].GetDimension() == 2 :
            plotZ = config[keyPlot]['plot']['z']
    
        for iHisto in range(len(histograms)):

            histograms[iHisto].SetTitle(";"+plotX[2]+";"+plotY[2])

            if iHisto == 0 :
                if histograms[iHisto].GetDimension() == 2 :
                    histograms[iHisto].Draw('colz')
                else :
                    histograms[iHisto].Draw('')
            else :
                histograms[iHisto].Draw('same')

            canvas.Update()

            if histograms[iHisto].ClassName() == "TEfficiency" and histograms[iHisto].GetDimension() == 1:
                histo = histograms[iHisto].GetPaintedGraph()
            elif histograms[iHisto].ClassName() == "TEfficiency" and histograms[iHisto].GetDimension() == 2:
                histo = histograms[iHisto].GetPaintedHistogram()
            else :
                histo = histograms[iHisto]
                
            if histograms[iHisto].ClassName() == "TEfficiency" :
                histo.GetXaxis().SetRangeUser(plotX[0], plotX[1])
                histo.GetYaxis().SetRangeUser(plotY[0], plotY[1])
                if histograms[iHisto].GetDimension() == 2 :
                    histo.SetMinimum(plotZ[0])
                    histo.SetMaximum(plotZ[1])
                    histo.Draw("colz")
            else:
                histo.GetYaxis().SetRangeUser(0.0, histo.GetMaximum() * 1.5)
                histo.GetXaxis().SetTitle(histoName)

            histo.GetXaxis().SetLabelSize(22)
            histo.GetXaxis().SetTitleFont(63)
            histo.GetXaxis().SetLabelFont(43)
            histo.GetXaxis().SetTitleSize(22)
            histo.GetXaxis().SetLabelSize(20)
            histo.GetXaxis().SetTitleOffset(1.2)
           
            histo.GetYaxis().SetLabelSize(22)
            histo.GetYaxis().SetTitleFont(63)
            histo.GetYaxis().SetLabelFont(43)
            histo.GetYaxis().SetTitleSize(22)
            histo.GetYaxis().SetLabelSize(20)
            histo.GetYaxis().SetTitleOffset(1.5)
            
            canvas.Update()

        canvas.Update()

        canvas.cd()

        if len(histograms) > 1 :
        
            leg = TLegend(0.49, 0.67, 0.75+0.1, 0.80)

            for iHisto in range(len(histograms)):
                leg.AddEntry(histograms[iHisto], inputLegendEntries[iHisto], 'LP')

            leg.SetBorderSize(1)
            leg.SetTextFont(43)
            leg.SetTextSize(20)
            leg.Draw()

        canvas.cd()
        latex = TLatex()
        latex.SetNDC()
        latex.SetTextFont(61)
        latex.SetTextSize(0.06)
        latex.DrawLatex(0.16, 0.82, config[keyPlot]['plot']['logo'][0])
        latex.SetTextFont(52)
        latex.SetTextSize(0.04)
        latex.SetTextAlign(11);
        latex.DrawLatex(0.16, 0.77, config[keyPlot]['plot']['logo'][1])
        latex.SetTextFont(42)
        latex.SetTextSize(0.038)
        latex.SetTextAlign(31);
        latex.DrawLatex(0.90, 0.91, config[keyPlot]['plot']['caption'])
        latex.SetTextAlign(11);
        latex.SetTextColor(1)
        latex.SetTextFont(43)
        latex.SetTextSize(20)
        latex.DrawLatex(0.49, 0.82, config[keyPlot]['plot']['legendTitle'])
        canvas.Update()
    
        # Save plot
        outputDirectory = config[keyPlot]['output']['directory']
        if args.verbosity==1:
            print('Output directory: {}'.format(outputDirectory))
        if not os.path.exists(outputDirectory):
            os.makedirs(outputDirectory)
        for fileType in config[keyPlot]['output']['fileType']:

            if config[keyPlot]['output']['filenamePlot'] == "all":
                 fileNamePlot = histoName
            else:
                 fileNamePlot = config[keyPlot]['output']['filenamePlot']
            
            canvas.SaveAs(os.path.join(outputDirectory,fileNamePlot+'.'+fileType))

    if args.verbosity==1:
        print('')
