HowTo for tests on Monitoring with Timeseries database


# Installation

## InfluxDb

1. Follow the instructions at https://docs.influxdata.com/influxdb/ \
   In my case I used the `legacy` version `v1.8` for which the instructions are at
   https://docs.influxdata.com/influxdb/v1.8/introduction/install/
1. Tune the options in `/etc/influxdb/influxdb.conf` \
   For example in my case I disabled the reporting of usage data to the developper and set the host address to
   `127.0.0.1:8086` in order to restrict connections to the test server to local ones.
   ```
   reporting-disabled = true
   [...]
   [http]
     bind-address = "127.0.0.1:8086"
   ```
1. Enable the InfluxDb service in the OS
   ```
   sudo -k systemctl enable --now influxdb
   systemctl status influxdb
   ```

## Grafana

1. Follow the instructions at https://grafana.com/docs/grafana/latest/setup-grafana/installation/
1. Tune the options in `/etc/grafana/grafana.ini` \
   For example in my case I set the host address to `127.0.0.1:3000` in order to restrict connections to the test server
   to local ones.
1. Enable the Grafana service in the OS
   ```
   sudo -k systemctl enable --now grafana-server.service
   systemctl status grafana-server.service
   ```

## Cbmroot

Nothing special to do, just normal compilation (and installation).

# Initialization

1. Create the target database in InfluxDb
   ```
   influx
   > create database monitoring_tests
   > quit
   ```
1. Enable the publishing of the monitoring data in the unpacking macro
   ```
   edit macro/run/run_unpack_tsa.C
   > Uncomment line 486
   ```
1. Setup connection to InfluxDb in Grafana
   1. Connect to `https://localhost:3000` in your favorite web browser (tested with Firefox >= 102)
   1. Connect as `admin:admin` and set the new admin password when requested (only first connection)
   1. Click on the three horizontal bars in the top left and then on `Connections > Connect Data`
   1. In the search bar enter `InfluxDb` and select `InfluxDB` (not `Influx Admin`!)
   1. Click on the new right button `create a InfluxDB data source` (blue if in Dark mode)
   1. In URL, enter `http://127.0.0.1:8086`
   1. Down in the page, in database enter `monitoring_tests`
   1. Click on `Save and Test` at the bottom of the page
1. Add the unpacker dashboard in Grafana
   1. Click on the three horizontal bars in the top left and then on `Dashboards`
   1. Click on the `New` button in the right (blue in Dark mode)
   1. Click on `Import` in the new menu which rolled down
   1. Select the file `macro/run/grafana_models/UnpackerPerfs.json`
   1. Click on `Import` at the bottom of the page

# Running and checking

1. In a first console, just run the unpacker macro with the typical command, everything will happen in the background
   ```
   root -l -b -q 'run_unpack_tsa.C(<TSA file>, <RUN ID>, <"" or setup name if not standard>, <-1 or NB TS>)'
   example > root -l -b -q 'run_unpack_tsa.C("/opt/cbm/beamtime-test-data/mcbm2022/2391_firstSpill.tsa",2391,"",-1)'
   ```
1. In a second console, check if the data arrive in the InfluxDB (this may take some time as entries are queued only
   every `1 s` in  data time and published only every `10 s` in processing time)
   ```
   influx
   > use monitoring_tests
   > select * from unpack_perf
   > select * from unpack_perf where "det" = "Bmon"
   ```
1. In you prefered web browser, connect to the grafana server and load the dashboard "Unpacker Performances"
   1. Click on the three horizontal bars in the top left and select `Dashboards`
   1. Click on the `General` folder in the center if it is not already expanded
   1. Click on `Unpacker Performances`
   1. Refresh the page to see more and more points appearing (for the 1st spill of mCBM run 2391, the unpacking time is
      around `600-700 s`)

# Exporting a Dashboard

To create/modify a dashboard, please refer to the graphana Documentation and the WebGUI itself.\
Once a new dashboard has be generated and tested locally, you can export it to JSON and share it through this cbmroot
git repository with the following steps:
1. Click on "Dashboard settings": the dented wheel close to the data picking GUI in the top bar
1. Click on `JSON model` at the bottom of the left menu
1. Click in the "online editor"
1. Select all, copy and paste in your favorite editor
1. Save as `<DashboardName>.json` in the folder `<cbmroot_source>/macro/run/graphana_models`
