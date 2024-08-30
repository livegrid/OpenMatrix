<script>
    export let value;
</script>

{#if value === 1}
    <span class="bg-emerald-200/50 text-emerald-800 text-xs font-medium me-2 px-3 py-1 rounded-full dark:bg-emerald-700/50 dark:text-emerald-300">Connected</span>
{:else if value === 2}
    <span class="bg-yellow-200/50 text-yellow-800 text-xs font-medium me-2 px-3 py-1 rounded-full dark:bg-yellow-700/50 dark:text-yellow-300">Reconnecting</span>
{:else if value === 3}
    <span class="bg-red-200/50 text-red-800 text-xs font-medium me-2 px-3 py-1 rounded-full dark:bg-red-700/50 dark:text-red-300">Failed</span>
{:else}
    <span class="bg-zinc-200/50 text-zinc-800 text-xs font-medium me-2 px-3 py-1 rounded-full dark:bg-zinc-700/50 dark:text-zinc-300">Disconnected</span>
{/if}