Highcharts.setOptions({
    time: {
        timezone: undefined,
    },
    lang: {
        locale: 'en-GB',
    },
});

const temperature_chart = Highcharts.chart('temperature-chart', {
    title: { text: 'Temperature' },
    plotOptions: {
        line: {
            dataLabels: { enabled: true }
        },
    },
    xAxis: {
        title: { text: 'Time' },
        type: 'datetime',
    },
    yAxis: {
        title: { text: 'Temperature (°C)' }
    },
    series: [{
        showInLegend: false,
        name: 'Degrees',
        data: [],
    }],
    credits: {
        enabled: false
    },
});

const humidity_chart = Highcharts.chart('humidity-chart', {
    title: { text: 'Humidity' },
    plotOptions: {
        line: {
            dataLabels: { enabled: true }
        },
    },
    xAxis: {
        title: { text: 'Time' },
        type: 'datetime',
    },
    yAxis: {
        title: { text: 'Humidity (%)' }
    },
    series: [{
        showInLegend: false,
        name: 'Percentage',
        data: [],
    }],
    credits: {
        enabled: false
    },
});

const pressure_chart = Highcharts.chart('pressure-chart', {
    title: { text: 'Barometric Pressure' },
    plotOptions: {
        line: {
            dataLabels: { enabled: true }
        },
    },
    xAxis: {
        title: { text: 'Time' },
        type: 'datetime',
    },
    yAxis: {
        title: { text: 'Barometric Pressure (hPa)' }
    },
    series: [{
        showInLegend: false,
        name: 'Hectopascals',
        data: [],
    }],
    credits: {
        enabled: false
    },
});

function roundNumber(num, n) {
    return Math.round(num * Math.pow(10, n)) / Math.pow(10, n);
}

const URL = 'http://weather-station.local';

async function updateCharts() {
    const url = `${URL}/api/weather`;
    try {
        const response = await fetch(url, { method: 'GET' });
        const data = await response.json();

        const timestamp = new Date().getTime();

        if(data.temperature) {
            const temperature = roundNumber(data.temperature, 2);
            temperature_chart.series[0].addPoint([timestamp, temperature], animation=true);
        }
        if(data.humidity) {
            const humidity = roundNumber(data.humidity, 2);
            humidity_chart.series[0].addPoint([timestamp, humidity], animation=true);
        }
        if(data.pressure) {
            const pressure = roundNumber(data.pressure, 2);
            pressure_chart.series[0].addPoint([timestamp, pressure], animation=true);
        }
    } catch (error) {
        console.error(error);
    }
}

updateCharts();
// every 2 seconds
setInterval(() => {
    updateCharts();
}, 2000);