<script>
    import Button from "./Button.svelte";

    export let selected, loading, absolute = true;
</script>

{#if loading}
    <Button loading={true} className={absolute ? 'absolute bottom-3 right-3' : ''} />
{:else}
    {#if selected}
        <div class={absolute ? 'absolute bottom-4 right-4' : ''}>
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-5 h-5 text-emerald-500"><circle cx="12" cy="12" r="10"/><path d="m9 12 2 2 4-4"/></svg>
            <!-- Selected -->
        </div>
    {:else}
        <Button loading={loading} on:click className={absolute ? 'absolute bottom-3 right-3' : ''}>
            <!-- <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-4 h-4"><circle cx="12" cy="12" r="10"/><path d="m9 12 2 2 4-4"/></svg> -->
            Select
        </Button>
    {/if}
{/if}