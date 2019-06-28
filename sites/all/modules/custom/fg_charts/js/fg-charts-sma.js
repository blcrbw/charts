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


            activity = JSON.parse(Drupal.settings.chartsJSONSMA);
            activity.datasets.forEach(function (dataset, i) {

                // Add X values
                dataset = Highcharts.map(dataset, function (val, j) {
                    return [activity.xData[j], val];
                });

                var chartDiv = document.createElement('div');
                chartDiv.className = 'chart';
                document.getElementById('container').appendChild(chartDiv);

                Highcharts.chart(chartDiv, {
                    chart: {
                        marginLeft: 20, // Keep all charts left aligned
                        spacingTop: 5,
                        spacingBottom: 5
                    },
                    title: {
                        text: '',
                        align: 'left',
                        margin: 0,
                        enabled: false,
                        x: 10
                    },
                    credits: {
                        enabled: false
                    },
                    legend: {
                        enabled: false
                    },
                    xAxis: {
                        crosshair: true,
                        events: {
                            setExtremes: syncExtremes
                        },
                        labels: {
                            format: '{value} s'
                        },
                        min: 0
                    },
                    yAxis: {
                        title: {
                            text: null
                        },
                        min: 0
                    },
                    tooltip: {
                        positioner: function () {
                            return {
                                // right aligned
                                x: this.chart.chartWidth - this.label.width,
                                y: 10 // align to title
                            };
                        },
                        borderWidth: 0,
                        backgroundColor: 'none',
                        pointFormat: '{point.y}',
                        headerFormat: '',
                        shadow: false,
                        enabled: false,
                        style: {
                            fontSize: '18px',
                            float: 'right'
                        },
                        valueDecimals: dataset.valueDecimals
                    },
                    series: [{
                        data: dataset.data,
                        // name: dataset.name,
                        // type: dataset.type,
                        color: Highcharts.getOptions().colors[i],
                        fillOpacity: 0.3,
                        tooltip: {
                            valueSuffix: ' ' + dataset.unit
                        }
                    }]
                });
            });

        }
    };

})(jQuery);
