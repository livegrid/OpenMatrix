
<script>
  import StatCard from '@/components/StatCard.svelte';
  import ChartCard from '@/components/ChartCard.svelte';

  import { state } from '@/store';

</script>
<!-- Your content -->
<div>
  <div class="flex flex-wrap items-center gap-6 sm:flex-nowrap sm:justify-between">
    <h1 class="text-base text-lg font-semibold leading-7 text-gray-900 dark:text-zinc-300">Environment</h1>
    <!-- <div class="order-last flex w-full gap-x-8 text-sm font-semibold leading-6 sm:order-none sm:w-auto sm:pl-6 sm:leading-7">
      <a href="#" class="text-emerald-600">Last 1 hour</a>
      <a href="#" class="text-gray-500 dark:text-zinc-400">Last 24 hours</a>
    </div> -->
  </div>
</div>

<div class="mt-8">
  <!-- <h3 class="text-base font-semibold leading-6 text-gray-900 dark:text-zinc-300">Last 7 days</h3> -->
  <dl class="grid grid-cols-1 gap-5 sm:grid-cols-1 lg:grid-cols-3">
    <StatCard title="Temperature" value={$state?.enviournment?.temperature?.value} symbol="°C" diff={$state?.enviournment?.temperature?.diff} icon='temperature' />
    <StatCard title="Humidity" value={$state?.enviournment?.humidity?.value} symbol="%" diff={$state?.enviournment?.humidity?.diff} icon='humidity' />
    <StatCard title="Carbon Dioxide" value={$state?.enviournment?.co2?.value} symbol="ppm" diff={$state?.enviournment?.co2?.diff} icon='co2' />
  </dl>
</div>
<div class="mt-5">
  <dl class="grid grid-cols-1 gap-5">
    <ChartCard title="Temperature" value="24" symbol="°C" trend="2" icon='temperature' />
    <ChartCard title="Humidity" value="24" symbol="°C" trend="2" icon='humidity' />
    <ChartCard title="Carbon Dioxide" value="24" symbol="°C" trend="2" icon='co2' />
  </dl>
</div>