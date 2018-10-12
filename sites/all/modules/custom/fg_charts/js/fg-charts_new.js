(function ($) {

    /**
     * Move a block in the blocks table from one region to another via select list.
     *
     * This behavior is dependent on the tableDrag behavior, since it uses the
     * objects initialized in that behavior to update the row.
     */
    Drupal.behaviors.fgCharts = {
        attach: function (context, settings) {

            /**
             * In order to synchronize tooltips and crosshairs, override the
             * built-in events with handlers defined on the parent element.
             */
            ['mousemove', 'touchmove', 'touchstart'].forEach(function (eventType) {
                document.getElementById('container').addEventListener(
                    eventType,
                    function (e) {
                        var chart,
                            point,
                            i,
                            event;

                        for (i = 0; i < Highcharts.charts.length; i = i + 1) {
                            chart = Highcharts.charts[i];
                            // Find coordinates within the chart
                            event = chart.pointer.normalize(e);
                            // Get the hovered point
                            point = chart.series[0].searchPoint(event, true);

                            if (point) {
                                point.highlight(e);
                            }
                        }
                    }
                );
            });

            /**
             * Override the reset function, we don't need to hide the tooltips and
             * crosshairs.
             */
            Highcharts.Pointer.prototype.reset = function () {
                return undefined;
            };

            /**
             * Highlight a point by showing tooltip, setting hover state and draw crosshair
             */
            Highcharts.Point.prototype.highlight = function (event) {
                event = this.series.chart.pointer.normalize(event);
                this.onMouseOver(); // Show the hover marker
                this.series.chart.tooltip.refresh(this); // Show the tooltip
                this.series.chart.xAxis[0].drawCrosshair(event, this); // Show the crosshair
            };

            /**
             * Synchronize zooming through the setExtremes event handler.
             */
            function syncExtremes(e) {
                var thisChart = this.chart;

                if (e.trigger !== 'syncExtremes') { // Prevent feedback loop
                    Highcharts.each(Highcharts.charts, function (chart) {
                        if (chart !== thisChart) {
                            if (chart.xAxis[0].setExtremes) { // It is null while updating
                                chart.xAxis[0].setExtremes(
                                    e.min,
                                    e.max,
                                    undefined,
                                    false,
                                    {trigger: 'syncExtremes'}
                                );
                            }
                        }
                    });
                }
            }


            activity = JSON.parse(Drupal.settings.chartsJSON);

          var detailChart;

          // create the detail chart
          function createDetail(masterChart) {
debugger;
            // prepare the detail chart
            var detailData = [],
              detailStart = activity.xData[0];

            $.each(masterChart.series[0].data, function () {
              if (this.x >= detailStart) {
                detailData.push(this.y);
              }
            });

            // create a detail chart referenced by a global variable
            detailChart = Highcharts.chart('detail-container', {
              chart: {
                marginBottom: 120,
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
                text: 'Historical USD to EUR Exchange Rate'
              },
              subtitle: {
                text: 'Select an area by dragging across the lower chart'
              },
              xAxis: {
                type: 'datetime'
              },
              yAxis: {
                title: {
                  text: null
                },
                maxZoom: 0.1
              },
              tooltip: {
                formatter: function () {
                  var point = this.points[0];
                  return '<b>' + point.series.name + '</b><br/>' + Highcharts.dateFormat('%A %B %e %Y', this.x) + ':<br/>' +
                    '1 USD = ' + Highcharts.numberFormat(point.y, 2) + ' EUR';
                },
                shared: true
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
                name: 'USD to EUR',
                pointStart: detailStart,
                pointInterval: 24 * 3600 * 1000,
                data: detailData
              }],

              exporting: {
                enabled: false
              }

            }); // return chart
          }

          // create the master chart
          function createMaster() {
            Highcharts.chart('master-container', {
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
                      detailData = [],
                      xAxis = this.xAxis[0];

                    // reverse engineer the last part of the data
                    $.each(this.series[0].data, function () {
                      if (this.x > min && this.x < max) {
                        detailData.push([this.x, 1]);
                      }
                    });

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


                    detailChart.series[0].setData(detailData);

                    return false;
                  }
                }
              },
              title: {
                text: null
              },
              xAxis: {
                type: 'datetime',
                showLastTickLabel: true,
                maxZoom: 5000,
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
                pointInterval: 24 * 3600 * 1000,
                pointStart: activity.xData[0],
                data: activity.xData
              }],

              exporting: {
                enabled: false
              }

            }, function (masterChart) {

                activity.datasets.forEach(function (dataset, i) {
                    debugger;
                    createDetail(masterChart);
                })
            }); // return chart instance
          }

          // make the container smaller and add a second container for the master chart
          var $container = $('#container')
            .css('position', 'relative');

          $('<div id="detail-container">')
            .appendTo($container);

          $('<div id="master-container">')
            .css({
              position: 'absolute',
              top: 300,
              height: 100,
              width: '100%'
            })
            .appendTo($container);

          // create master and in its callback, create the detail chart
          createMaster();
            // activity.datasets.forEach(function (dataset, i) {
            //
            //     // Add X values
            //     dataset.data = Highcharts.map(dataset.data, function (val, j) {
            //         return [activity.xData[j], val];
            //     });
            //
            //     var chartDiv = document.createElement('div');
            //     chartDiv.className = 'chart';
            //     document.getElementById('container').appendChild(chartDiv);
            //
            //     Highcharts.chart(chartDiv, {
            //         chart: {
            //             marginLeft: 20, // Keep all charts left aligned
            //             spacingTop: 5,
            //             spacingBottom: 5
            //         },
            //         title: {
            //             text: '',
            //             align: 'left',
            //             margin: 0,
            //             enabled: false,
            //             x: 10
            //         },
            //         credits: {
            //             enabled: false
            //         },
            //         legend: {
            //             enabled: false
            //         },
            //         xAxis: {
            //             crosshair: true,
            //             events: {
            //                 setExtremes: syncExtremes
            //             },
            //             labels: {
            //                 format: '{value} s'
            //             },
            //             min: 0
            //         },
            //         yAxis: {
            //             title: {
            //                 text: null
            //             },
            //             min: 0
            //         },
            //         tooltip: {
            //             positioner: function () {
            //                 return {
            //                     // right aligned
            //                     x: this.chart.chartWidth - this.label.width,
            //                     y: 10 // align to title
            //                 };
            //             },
            //             borderWidth: 0,
            //             backgroundColor: 'none',
            //             pointFormat: '{point.y}',
            //             headerFormat: '',
            //             shadow: false,
            //             enabled: false,
            //             style: {
            //                 fontSize: '18px',
            //                 float: 'right'
            //             },
            //             valueDecimals: dataset.valueDecimals
            //         },
            //         series: [{
            //             data: dataset.data,
            //             name: dataset.name,
            //             type: dataset.type,
            //             color: Highcharts.getOptions().colors[i],
            //             fillOpacity: 0.3,
            //             tooltip: {
            //                 valueSuffix: ' ' + dataset.unit
            //             }
            //         }]
            //     });
            // });

        }
    };

})(jQuery);
