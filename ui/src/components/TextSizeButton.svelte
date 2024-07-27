<script>
    export let name, selected;
</script>

<button type="button" on:click class={`inline-flex justify-center items-center w-8 h-8 rounded cursor-pointer ${ selected ? 'text-emerald-700 bg-emerald-50/50 dark:bg-emerald-950/50 dark:text-emerald-600 border border-emerald-600' : 'text-zinc-500 hover:text-zinc-900 hover:bg-zinc-100 dark:text-zinc-400 dark:hover:text-white dark:hover:bg-zinc-900'}`}>
    {name}
</button>