<script>
  export let title, value, symbol, icon, trend, trendInverse = false;
  import { theme } from '@/store';
  import Icon from "./Icon.svelte";
  import { Line } from 'svelte-chartjs'

  $: trendColor = trendInverse ? 'text-red-500' : 'text-emerald-600';

  function getRandomInt(min, max) {
  const minCeiled = Math.ceil(min);
  const maxFloored = Math.floor(max);
  return Math.floor(Math.random() * (maxFloored - minCeiled) + minCeiled); // The maximum is exclusive and the minimum is inclusive
}
  const randomValue = () => {
    // Return random value between min 20 and max 46
    return getRandomInt(20, 46);
  }

  const getDate = (hours) => {
    // Add hours to current time starting from 00:00
    const date = new Date();
    date.setHours(hours);
    date.setMinutes(0);
    date.setSeconds(0);
    date.setMilliseconds(0);
    return date
  };

  let result = [
    {
      x: getDate(0),
      y: 24
    },
    {
      x: getDate(1),
      y: 24
    },
    {
      x: getDate(2),
      y: 25
    },
    {
      x: getDate(3),
      y: 25
    },
    {
      x: getDate(4),
      y: 26
    },
    {
      x: getDate(5),
      y: 26
    },
    {
      x: getDate(6),
      y: 26
    },
    {
      x: getDate(7),
      y: 28
    },
    {
      x: getDate(8),
      y: 29
    },
    {
      x: getDate(9),
      y: 30
    },
    {
      x: getDate(10),
      y: 31
    },
    {
      x: getDate(11),
      y: 32
    },
    {
      x: getDate(12),
      y: 34
    },
    {
      x: getDate(13),
      y: 35
    },
    {
      x: getDate(14),
      y: 38
    },
    {
      x: getDate(15),
      y: 36
    },
    {
      x: getDate(16),
      y: 34
    },
    {
      x: getDate(17),
      y: 30
    },
    {
      x: getDate(18),
      y: 28
    },
    {
      x: getDate(19),
      y: 28
    },
    {
      x: getDate(20),
      y: 27
    },
    {
      x: getDate(21),
      y: 25
    },
    {
      x: getDate(22),
      y: 25
    },
    {
      x: getDate(23),
      y: 24
    },
    {
      x: getDate(24),
      y: 24
    },
  ];

  console.log(result)

  export const data = {
    labels: result.map(d => d.x),
    datasets: [
      {
        label: '',
        fill: false,
        lineTension: 0.3,
        backgroundColor: 'rgb(7,150,105)',
        borderColor: 'rgb(7,150,105)',
        borderCapStyle: 'butt',
        borderDash: [],
        borderDashOffset: 0.0,
        borderJoinStyle: 'miter',
        pointBorderColor: 'rgb(7,150,105)',
        pointBackgroundColor: 'rgb(7,150,105)',
        // pointBorderWidth: 4,
        // pointHoverRadius: 4,
        pointHoverBackgroundColor: 'rgb(7,150,105)',
        // pointHoverBorderColor: 'rgba(220, 220, 220,1)',
        // pointHoverBorderWidth: 2,
        pointRadius: 3,
        pointHitRadius: 10,
        data: result.map(d => d.y),
      }
    ],
  };

  $: options = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: {
        display: false,
      },
    },
    scales: {
      x: {
        type: 'timeseries',
        // time: {
        //   unit: 'hour',
        //   displayFormats: {
        //     hour: 'HH:mm'
        //   }
        // },
        grid: {
          color: $theme === 'dark' ? 'rgb(24, 24, 27)' : 'rgb(229, 231, 235)'
        }
      },
      y: {
        grid: {
          color: $theme === 'dark' ? 'rgb(24, 24, 27)' : 'rgb(229, 231, 235)'
        }
      },
    }
  };
</script>

<div class="relative overflow-hidden rounded-md border border-gray-200 dark:border-zinc-900 bg-white/80 dark:bg-black/30 px-4 py-5 sm:px-6 sm:py-6">
  <dt class="flex items-center gap-x-4">
    <div class="rounded-md bg-emerald-600 p-2">
      <Icon name={icon} className="h-4 w-4 text-white" />
    </div>
    <p class="truncate font-medium text-gray-500">{ title }</p>
  </dt>
  <dd class="flex items-baseline mt-4 h-[260px]">
    <Line {data} options={options} />
  </dd>
</div>