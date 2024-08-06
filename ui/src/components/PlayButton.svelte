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
            <button disabled class="flex flex-row items-center uppercase gap-x-3 text-emerald-600 dark:text-emerald-700">
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="currentColor" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-4 h-4"><polygon points="6 3 20 12 6 21 6 3"/></svg>
            Playing
            </button>
        {:else}
            <button on:click class="flex flex-row items-center uppercase gap-x-3 text-gray-400 hover:text-gray-500 transition-colors">
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="currentColor" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-4 h-4"><polygon points="6 3 20 12 6 21 6 3"/></svg>
            Play
            </button>
        {/if}
    {/if}
{/if}