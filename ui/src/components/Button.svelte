<script>
    import Spinner from "./Spinner.svelte";

    export let type = 'button', loading = false, disabled = false, className = '', size = '';
</script>

<button type={type} disabled={loading || disabled} on:click class={`py-1 px-1.5 flex flex-row items-center ${ size === 'l' ? 'text-sm gap-x-1.5' : 'text-xs gap-x-1'} rounded-full bg-gray-50 dark:bg-zinc-900 border border-gray-300 dark:border-zinc-700 text-gray-500 ${ !loading && !disabled ? 'hover:text-gray-500 dark:hover:text-zinc-400 dark:hover:bg-zinc-800 dark:hover:border-zinc-600 hover:bg-gray-100 hover:border-gray-400' : '' } dark:text-zinc-500 transition-colors disabled:opacity-80 ${ className }`}>
    {#if loading}
        <Spinner size={size === 'l' ? 'm' : 's'} />
    {:else}
        <slot />
    {/if}
</button>