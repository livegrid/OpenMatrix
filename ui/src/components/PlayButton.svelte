<script>
    import StatSkeleton from "@/components/StatSkeleton.svelte";
    import Spinner from "@/components/Spinner.svelte";

    export let initializing, loading, active;
</script>

{#if initializing}
    <StatSkeleton />
{:else}
    {#if loading}
        <Spinner size="m" />
    {:else}
        {#if active}
            <button disabled class="py-1 px-2 flex flex-row items-center text-sm gap-x-1.5 rounded-full bg-emerald-50 dark:bg-emerald-900/40 border border-emerald-500 dark:border-emerald-700 text-emerald-600 dark:text-emerald-500 disabled:opacity-80">
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="currentColor" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-4 h-4"><polygon points="6 3 20 12 6 21 6 3"/></svg>
                Playing
            </button>
        {:else}
            <button on:click class="py-1 px-2 flex flex-row items-center text-sm gap-x-1.5 rounded-full bg-gray-50 dark:bg-zinc-900 border border-gray-300 hover:bg-gray-100 hover:border-gray-400 dark:border-zinc-700 text-gray-500 dark:text-zinc-500 hover:text-gray-500 dark:hover:text-zinc-400 dark:hover:bg-zinc-800 dark:hover:border-zinc-600 transition-colors disabled:opacity-80">
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="currentColor" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-4 h-4"><polygon points="6 3 20 12 6 21 6 3"/></svg>
                Play
            </button>
        {/if}
    {/if}
{/if}