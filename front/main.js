Highcharts.setOptions({
    time: {
        timezone: undefined,
    },
    lang: {
        locale: 'en-GB',
    },
});

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
        data: [],
    }],
    credits: {
        enabled: false
    },
});

function roundNumber(num, n) {
    return Math.round(num * Math.pow(10, n)) / Math.pow(10, n);
}

const URL = 'http://192.168.0.73';

async function updateChart() {
    const url = `${URL}/temperature`;
    try {
        const response = await fetch(url, {
            method: 'GET',
        });
        const data = await response.json();
        console.log(data);

        if(data.temperature) {
            const temperature = roundNumber(data.temperature, 2);
            const time = new Date().getTime();
            chart.series[0].addPoint([time, temperature], animation=true);
        }
    } catch (error) {
        console.error(error);
    }
}

updateChart();
// every 2 seconds
setInterval(() => {
    updateChart();
}, 2000);