(function ($) {

  /**
   * Move a block in the blocks table from one region to another via select list.
   *
   * This behavior is dependent on the tableDrag behavior, since it uses the
   * objects initialized in that behavior to update the row.
   */
  Drupal.behaviors.fgCharts = {
    attach: function (context, settings) {

      activity = JSON.parse(Drupal.settings.chartsJSON);

      var detailCharts = [];

      // create the detail chart
      function createDetail(masterChart, dataset) {
        // prepare the detail chart
        var detailData = [],
          detailStart = dataset.data[0][0];

        $.each(dataset.data, function () {
          if (this[0] >= detailStart) {
            detailData.push([this[0], this[1]]);
          }
        });

        var chartDiv = document.createElement('div');
        chartDiv.className = 'chart';
        document.getElementById('container').appendChild(chartDiv);
        // create a detail chart referenced by a global variable
        var detailChart = Highcharts.chart(chartDiv, {
          chart: {
            marginBottom: 20,
            reflow: false,
            marginLeft: 50,
            marginRight: 20,
            style: {
              position: 'absolute'
            }
          },
          credits: {
            enabled: false
          },
          title: {
            text: null
          },
          xAxis: {
            type: 'line',
            showLastTickLabel: true,
            maxZoom: 0.1,
            plotBands: [{
              id: 'mask-before',
              from: dataset.data[0],
              to: dataset.data[dataset.data.length - 1],
              color: 'rgba(0, 0, 0, 0.2)'
            }],
            title: {
              text: null
            }
          },
          yAxis: {
            title: {
              text: null
            },
            maxZoom: 0.1
          },
          tooltip: {
            enabled: false
          },
          legend: {
            enabled: false
          },
          plotOptions: {
            series: {
              marker: {
                enabled: false,
                states: {
                  hover: {
                    enabled: true,
                    radius: 3
                  }
                }
              }
            }
          },
          series: [{
            name: dataset.name,
            pointStart: detailStart,
            pointInterval: 24 * 3600 * 1000,
            data: detailData
          }],

          exporting: {
            enabled: false
          }

        });
        detailCharts.push(detailChart)
        //detailChart = detailChart;
        // return chart
      }

      // create the master chart
      function createMaster() {
        Highcharts.chart('master-container-one', {
          chart: {
            reflow: false,
            borderWidth: 0,
            backgroundColor: null,
            marginLeft: 50,
            marginRight: 20,
            zoomType: 'x',
            events: {
              // listen to the selection event on the master chart to update the
              // extremes of the detail chart
              selection: function (event) {
                var extremesObject = event.xAxis[0],
                  min = extremesObject.min,
                  max = extremesObject.max,
                  xAxis = this.xAxis[0];
                $.each(detailCharts, function (index) {
                  var detailData = []
                  debugger;

                  // reverse engineer the last part of the data
                  $.each(activity.datasets[index].data, function () {
                    if (this.x != undefined) {
                      if (this.x > min && this.x < max) {
                        detailData.push([this.x, this.y]);
                      }
                    }
                    if (this[0] != undefined) {
                      if (this[0] > min && this[0] < max) {
                        detailData.push([this[0], this[1]]);
                      }
                    }
                  });

                  this.series[0].setData(detailData);
                })


                // move the plot bands to reflect the new detail span
                xAxis.removePlotBand('mask-before');
                xAxis.addPlotBand({
                  id: 'mask-before',
                  from: activity.xData[0],
                  to: min,
                  color: 'rgba(0, 0, 0, 0.2)'
                });

                xAxis.removePlotBand('mask-after');
                xAxis.addPlotBand({
                  id: 'mask-after',
                  from: max,
                  to: activity.xData[activity.xData.length - 1],
                  color: 'rgba(0, 0, 0, 0.2)'
                });

                return false;
              }
            }
          },
          title: {
            text: null
          },
          xAxis: {
            type: 'line',
            showLastTickLabel: true,
            maxZoom: 0.1,
            plotBands: [{
              id: 'mask-before',
              from: activity.xData[0],
              to: activity.xData[activity.xData.length - 1],
              color: 'rgba(0, 0, 0, 0.2)'
            }],
            title: {
              text: null
            }
          },
          yAxis: {
            gridLineWidth: 0,
            labels: {
              enabled: false
            },
            title: {
              text: null
            },
            min: 0,
            showFirstLabel: false
          },
          tooltip: {
            formatter: function () {
              return false;
            }
          },
          legend: {
            enabled: false
          },
          credits: {
            enabled: false
          },
          plotOptions: {
            series: {
              fillColor: {
                linearGradient: [0, 0, 0, 70],
                stops: [
                  [0, Highcharts.getOptions().colors[0]],
                  [1, 'rgba(255,255,255,0)']
                ]
              },
              lineWidth: 1,
              marker: {
                enabled: false
              },
              shadow: false,
              states: {
                hover: {
                  lineWidth: 1
                }
              },
              enableMouseTracking: false
            }
          },
          series: [{
            type: 'area',
            name: 'time',
            pointInterval: (activity.xData[1] - activity.xData[0]) * 1000,
            pointStart: activity.xData[0],
            data: Highcharts.map(activity.xData, function (val, j) {
              return [activity.xData[j], 1];
            })
          }],
          exporting: {
            enabled: false
          }
        }, function (masterChart) {
          activity.datasets.forEach(function (dataset, i) {
            dataset.data = Highcharts.map(dataset.data, function (val, j) {
              return [activity.xData[j], val];
            });
            masterChart.series[0].data = dataset.data;
            createDetail(masterChart, dataset);
          })
        }); // return chart instance
      }

      // make the container smaller and add a second container for the master chart
      var $container = $('#container')
        .css('position', 'relative');

      $('<div id="master-container-one" class="chart">')
        .appendTo($container);

      // create master and in its callback, create the detail chart
      createMaster();

    }
  };

})(jQuery);
