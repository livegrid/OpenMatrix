<script>
    import StatSkeleton from "@/components/StatSkeleton.svelte";
    import Spinner from "@/components/Spinner.svelte";
    import Button from "./Button.svelte";

    export let initializing, loading, active;
</script>

{#if initializing}
    <StatSkeleton />
{:else}
    {#if loading}
        <Spinner size="m" />
    {:else}
        <Button on:click disabled={active} className={active ? 'border-emerald-500 dark:border-emerald-500 px-3' : 'px-3'} size='l'>
            {#if active}
                <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-4 h-4 text-emerald-500"><path d="M20 6 9 17l-5-5"/></svg>
                <span class="text-emerald-500">Playing</span>
            {:else}
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="currentColor" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-4 h-4"><polygon points="6 3 20 12 6 21 6 3"/></svg>
                Play
            {/if}
        </Button>
    {/if}
{/if}