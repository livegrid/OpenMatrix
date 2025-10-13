<script>
  import StatSkeleton from '@/components/StatSkeleton.svelte';
  import Icon from "./Icon.svelte";
  export let title, value, symbol, icon, diff;

  let trendColor;

  $: {
    if (diff?.type == 0) {
      trendColor = 'text-gray-500';
    } else if (diff?.type == 1) {
      if (diff?.inverse) {
        trendColor = 'text-red-500';
      } else {
        trendColor = 'text-emerald-500';
      }
    } else {
      if (diff?.inverse) {
        trendColor = 'text-emerald-500';
      } else {
        trendColor = 'text-red-500';
      }
    }
  }
</script>

<div class="relative overflow-hidden rounded-md border border-gray-200 dark:border-zinc-900 bg-white/80 dark:bg-black/30 px-4 py-5 sm:px-6 sm:py-6">
  <dt>
    <div class="absolute rounded-md bg-emerald-600 p-3">
      <Icon name={icon} className="h-6 w-6 text-white" />
    </div>
    <p class="ml-16 truncate text-sm font-medium text-gray-500">{ title }</p>
  </dt>
  <dd class="ml-16 flex items-baseline">
    <p class="text-2xl font-semibold text-gray-900 dark:text-zinc-200">
      {#if value == null || value == undefined}
        <StatSkeleton />
      {:else}
        {Number.isInteger(value) ? value : value.toFixed(1)} {symbol}
      {/if}
    </p>
    {#if (diff !== null && diff !== undefined) && diff?.type !== 0}
      <p class={`ml-2 flex items-baseline text-sm font-semibold ${ trendColor }`}>
        {#if diff?.type == 1}
          <svg class={`h-4 w-4 flex-shrink-0 self-center ${ trendColor }`} viewBox="0 0 20 20" fill="currentColor" aria-hidden="true">
            <path fill-rule="evenodd" d="M10 3a.75.75 0 01.75.75v10.638l3.96-4.158a.75.75 0 111.08 1.04l-5.25 5.5a.75.75 0 01-1.08 0l-5.25-5.5a.75.75 0 111.08-1.04l3.96 4.158V3.75A.75.75 0 0110 3z" clip-rule="evenodd" />
          </svg>
        {:else if diff?.type == 2}
          <svg class={`h-4 w-4 flex-shrink-0 self-center ${ trendColor }`} viewBox="0 0 20 20" fill="currentColor" aria-hidden="true">
            <path fill-rule="evenodd" d="M10 17a.75.75 0 01-.75-.75V5.612L5.29 9.77a.75.75 0 01-1.08-1.04l5.25-5.5a.75.75 0 011.08 0l5.25 5.5a.75.75 0 11-1.08 1.04l-3.96-4.158V16.25A.75.75 0 0110 17z" clip-rule="evenodd" />
          </svg>
        {/if}
        {#if diff?.type !== 0}
          <!-- trend should always be positive -->
          {Math.abs(diff?.value)} {symbol}
        {/if}
      </p>
    {/if}
  </dd>
</div>