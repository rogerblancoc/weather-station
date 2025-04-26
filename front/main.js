const chart = Highcharts.chart('temperature-chart', {
    title: { text: 'Temperature' },
    plotOptions: {
        line: {
            dataLabels: { enabled: true }
        },
    },
    xAxis: {
        type: 'datetime',
    },
    yAxis: {
        title: { text: 'Temperature' }
    },
    series: [{
        showInLegend: false,
        data: [
            [10, 20],
            [11, 22],
            [12, 30],
            [13, 25],
        ]
    }],
    credits: {
        enabled: false
    },
});

// Simulate new data points
setTimeout(() => {
    chart.series[0].addPoint([14, 28], animation=true);
    chart.series[0].addPoint([15, 26], animation=true);
}, 2000);
setTimeout(() => {
    chart.series[0].addPoint([16, 28], shift=true, animation=true);
}, 4000);
setTimeout(() => {
    chart.series[0].addPoint([17, 26], animation=true);
}, 8000);
