<script>
    import Spinner from "./Spinner.svelte";

    export let selected, loading;
</script>

{#if loading}
    <button disabled={loading} class="absolute bottom-3 right-3 py-1 px-1.5 flex flex-row items-center text-xs gap-x-1 rounded-full bg-gray-50 dark:bg-zinc-900 border border-gray-300 dark:border-zinc-700 text-gray-500 dark:text-zinc-500 transition-colors disabled:opacity-80">
        <Spinner size="s" />
    </button>
{:else}
    {#if selected}
        <button disabled class="absolute bottom-3 right-3 py-1 px-1.5 flex flex-row items-center text-xs gap-x-1 rounded-full text-emerald-600">
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-5 h-5"><circle cx="12" cy="12" r="10"/><path d="m9 12 2 2 4-4"/></svg>
            <!-- Selected -->
        </button>
    {:else}
        <button disabled={loading} on:click class="absolute bottom-3 right-3 py-1 px-1.5 flex flex-row items-center text-xs gap-x-1 rounded-full bg-gray-50 dark:bg-zinc-900 border border-gray-300 hover:bg-gray-100 hover:border-gray-400 dark:border-zinc-700 text-gray-500 dark:text-zinc-500 hover:text-gray-500 dark:hover:text-zinc-400 dark:hover:bg-zinc-800 dark:hover:border-zinc-600 transition-colors disabled:opacity-80">
            <!-- <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-4 h-4"><circle cx="12" cy="12" r="10"/><path d="m9 12 2 2 4-4"/></svg> -->
            Select
        </button>
    {/if}
{/if}