import pandas as pd
import datetime as dt
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.dates as mdates

#%%

path = "OneMonth.csv"

class CLIMATE():
    def __init__(self,df):
        self.df = df
        self.temperature = self.df.iloc[:,0]
        self.humidity = self.df.iloc[:,1]
        self.lux = self.df.iloc[:,2]
        self.co2 = self.df.iloc[:,3]
        self.xax = self.converttime()
        
    def converttime(self):
        self.temparr = []      # temporaray array per month
        for i in range(len(self.df)):
            self.temparr.append(
                dt.datetime(
                    int(self.df.iloc[i,4]),int(self.df.iloc[i,5]),
                    int(self.df.iloc[i,6]),int(self.df.iloc[i,7]),
                    int(self.df.iloc[i,8])))
        return self.temparr

def Plotdata():
    # plot monthly
    df = pd.read_csv(path)
    clima = CLIMATE(df)
    
    f = plt.figure(figsize=(10*1.5,6*1.5))
    # row, column, position
    axtemp = f.add_subplot(221)
    axhum = f.add_subplot(222, sharex=axtemp)
    axlux = f.add_subplot(223, sharex=axhum)
    axco2 = f.add_subplot(224, sharex=axlux)
    
    # to make space for the titles
    plt.subplots_adjust(left=None, bottom=None, right=None, top=None, 
                        wspace=None, hspace=0.7)
    
    
    # axis for temperature
    axtemp.set_ylabel("Temperatur in °C", fontsize = 10)
    axtemp.set_xlabel("Zeit", fontsize = 10)
    axtemp.set_title("Temperatur", fontsize = 10)
    axtemp.tick_params(direction = "in")
    axtemp.plot(clima.xax, clima.temperature, color = "tab:red", 
                label = "Temperatur", 
                linewidth = "1")
    axtemp.grid()
    axtemp.tick_params(direction = "in")
    axtemp.xaxis.set_major_formatter(mdates.DateFormatter("%d/%m-%H$\,$Uhr"))
    for label in axtemp.get_xticklabels(which='major'):
        label.set(rotation=30, horizontalalignment='right')
    legend = axtemp.legend(loc = "best", shadow = True, fontsize = 10)
    
    
    # axis for humidity
    axhum.set_ylabel("Luftfeuchtigkeit in %", fontsize = 10)
    axhum.set_xlabel("Zeit", fontsize = 10)
    axhum.set_title("Luftfeuchtigkeit", fontsize = 10)
    axhum.tick_params(direction = "in")
    axhum.plot(clima.xax, clima.humidity, color = "tab:blue", 
               label = "Luftfeuchtigkeit", 
               linewidth = "1")
    axhum.grid()
    axhum.tick_params(direction = "in")
    axhum.xaxis.set_major_formatter(mdates.DateFormatter("%d/%m-%H$\,$Uhr"))
    for label in axhum.get_xticklabels(which='major'):
        label.set(rotation=30, horizontalalignment='right')
    
    # inserting the ok-threshhold for humidity
    axhum.fill_between(clima.xax,np.full((len(clima.xax),1),60)[:,0],40,
                        alpha=0.2, color = "forestgreen")
    
    legend = axhum.legend(loc = "best", shadow = True, fontsize = 10)
    
    
    # axis for intensity of light
    axlux.plot(clima.xax, clima.humidity, color = "tab:orange", label = "Lux", 
              linewidth = "1")
    axlux.grid()
    axlux.set_title("Lichtintensität", fontsize = 10)
    axlux.tick_params(direction = "in")
    axlux.xaxis.set_major_formatter(mdates.DateFormatter("%d/%m-%H$\,$Uhr"))
    axlux.set_ylabel("Licht in Pseudowerte", fontsize = 10)
    axlux.set_xlabel("Zeit", fontsize = 10)
    for label in axlux.get_xticklabels(which='major'):
        label.set(rotation=30, horizontalalignment='right')
    
    legend = axlux.legend(loc = "best", shadow = True, fontsize = 10)
    
    
    # axis for co2
    axco2.set_ylabel("CO2 in PPM (Parts per Million)", fontsize = 10)
    axco2.set_xlabel("Zeit", fontsize = 10)
    axco2.set_title("CO2 Konzentration", fontsize = 10)
    axco2.tick_params(direction = "in")
    axco2.plot(clima.xax, clima.co2, color = "tab:brown", 
                 label = "PPM", 
                 linewidth = "1")
    axco2.grid()
    axco2.tick_params(direction = "in")
    axco2.xaxis.set_major_formatter(mdates.DateFormatter("%d/%m-%H$\,$Uhr"))
    for label in axco2.get_xticklabels(which='major'):
        label.set(rotation=30, horizontalalignment='right')
    
    # inserting ok-threshhold for co2
    axco2.fill_between(clima.xax,np.full(len(clima.xax),800),0,
                        alpha=0.2, color = "forestgreen")

    axco2.fill_between(clima.xax,np.full(len(clima.xax),1200),800,
                        alpha=0.2, color = "darkorange")

    axco2.fill_between(clima.xax,np.full(len(clima.xax),max(clima.co2)),1200,
                        alpha=0.2, color = "red")
    legend = axco2.legend(loc = "best", shadow = True, fontsize = 10)
    
    plt.show()
    
Plotdata()






















