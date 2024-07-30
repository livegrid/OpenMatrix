<script>
  // @ts-nocheck
  import Icon from "./Icon.svelte";
  import { Line } from 'svelte-chartjs'
  import { theme } from '@/store';

  export let title, value, symbol, icon;

  function generateLast24Hours() {
    let result = [];
    const now = new Date();

    for (let i = 0; i < 24; i++) {
      const date = new Date(now);
      date.setHours(date.getHours() - i);
      date.setMinutes(0, 0, 0); // Set minutes, seconds, and milliseconds to 00:00:00.000
      result.push(date);
    }
    result.reverse();
    return result;
  }

  $: data = {
    labels: generateLast24Hours(),
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
        pointRadius: 0,
        pointHitRadius: 10,
        data: Array.isArray(value) ? value : [],
      }
    ],
  };

  $: options = {
    animation: false,
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: {
        display: false,
      },
      tooltip: {
        callbacks: {
          label(tooltipItems) {
            return `${tooltipItems.formattedValue} ${symbol}`;
          }
        }
      }
    },
    scales: {
      x: {
        type: 'timeseries',
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
  {#if value === null || value === undefined || !Array.isArray(value) || value.length === 0}
    <dd class="flex flex-col gap-4 items-center justify-center mt-4 h-[260px]">
      <div role="status">
          <svg aria-hidden="true" class="w-8 h-8 text-gray-200 animate-spin dark:text-gray-600 fill-emerald-600" viewBox="0 0 100 101" fill="none" xmlns="http://www.w3.org/2000/svg">
              <path d="M100 50.5908C100 78.2051 77.6142 100.591 50 100.591C22.3858 100.591 0 78.2051 0 50.5908C0 22.9766 22.3858 0.59082 50 0.59082C77.6142 0.59082 100 22.9766 100 50.5908ZM9.08144 50.5908C9.08144 73.1895 27.4013 91.5094 50 91.5094C72.5987 91.5094 90.9186 73.1895 90.9186 50.5908C90.9186 27.9921 72.5987 9.67226 50 9.67226C27.4013 9.67226 9.08144 27.9921 9.08144 50.5908Z" fill="currentColor"/>
              <path d="M93.9676 39.0409C96.393 38.4038 97.8624 35.9116 97.0079 33.5539C95.2932 28.8227 92.871 24.3692 89.8167 20.348C85.8452 15.1192 80.8826 10.7238 75.2124 7.41289C69.5422 4.10194 63.2754 1.94025 56.7698 1.05124C51.7666 0.367541 46.6976 0.446843 41.7345 1.27873C39.2613 1.69328 37.813 4.19778 38.4501 6.62326C39.0873 9.04874 41.5694 10.4717 44.0505 10.1071C47.8511 9.54855 51.7191 9.52689 55.5402 10.0491C60.8642 10.7766 65.9928 12.5457 70.6331 15.2552C75.2735 17.9648 79.3347 21.5619 82.5849 25.841C84.9175 28.9121 86.7997 32.2913 88.1811 35.8758C89.083 38.2158 91.5421 39.6781 93.9676 39.0409Z" fill="currentFill"/>
          </svg>
          <span class="sr-only">Fetching...</span>
      </div>
        <h1 class="text-zinc-300 text-xs uppercase">
          Fetching
        </h1>
    </dd>
  {:else}
    <dd class="flex items-baseline mt-4 h-[260px]">
      <Line {data} options={options} />
    </dd>
  {/if}
</div>