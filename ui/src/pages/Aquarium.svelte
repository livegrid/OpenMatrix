
<script>
  import StatCard from '@/components/StatCard.svelte';
  import ChartCard from '@/components/ChartCard.svelte';
  import ModePageLayout from '@/components/ModePageLayout.svelte';
  import { state } from '@/store';
</script>
<!-- Your content -->
<ModePageLayout name={'Aquarium'} id={0}>
  <div class="mt-8">
    <!-- <h3 class="text-base font-semibold leading-6 text-gray-900 dark:text-zinc-300">Last 7 days</h3> -->
    <dl class="grid grid-cols-1 gap-5 sm:grid-cols-1 lg:grid-cols-3">
      {#if $state?.temperatureUnit == 0}
        <StatCard title="Temperature" value={$state?.environment?.temperature?.value} symbol="°C" diff={$state?.environment?.temperature?.diff} icon='temperature' />
      {:else}
        <StatCard title="Temperature" value={$state?.environment?.temperature_fahrenheit?.value} symbol="°F" diff={$state?.environment?.temperature_fahrenheit?.diff} icon='temperature' />
      {/if}
      <StatCard title="Humidity" value={$state?.environment?.humidity?.value} symbol="%" diff={$state?.environment?.humidity?.diff} icon='humidity' />
      <StatCard title="Carbon Dioxide" value={$state?.environment?.co2?.value} symbol="ppm" diff={$state?.environment?.co2?.diff} icon='co2' />
    </dl>
  </div>
  <div class="mt-5">
    <dl class="grid grid-cols-1 gap-5">
      {#if $state?.temperatureUnit == 0}
        <ChartCard title="Temperature" value={$state?.environment?.temperature?.history_24h} symbol="°C" icon='temperature' />
      {:else}
        <ChartCard title="Temperature" value={$state?.environment?.temperature_fahrenheit?.history_24h} symbol="°F" icon='temperature' />
      {/if}
      <ChartCard title="Humidity" value={$state?.environment?.humidity?.history_24h} symbol="%" icon='humidity' />
      <ChartCard title="Carbon Dioxide" value={$state?.environment?.co2?.history_24h} symbol="ppm" icon='co2' />
    </dl>
  </div>
</ModePageLayout>
