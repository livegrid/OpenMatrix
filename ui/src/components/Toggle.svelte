<script>
  import { createEventDispatcher } from "svelte";

  const dispatch = createEventDispatcher();

  export let checked;

  const toggle = () => {
    let value = !checked;
    dispatch('change', value);
  };
</script>

<button on:click={toggle} on:mouseup type="button" class="group relative inline-flex h-5 w-10 flex-shrink-0 cursor-pointer items-center justify-center rounded-full focus:outline-none focus:ring-2 focus:ring-indigo-600 focus:ring-offset-2" role="switch" aria-checked="false">
  <span aria-hidden="true" class="pointer-events-none absolute h-full w-full rounded-md bg-transparent"></span>
  <span aria-hidden="true" class={`${ checked ? 'bg-emerald-600 dark:bg-emerald-600' : 'bg-gray-200 dark:bg-zinc-800' } pointer-events-none absolute mx-auto h-4 w-9 rounded-full transition-colors duration-200 ease-in-out`}></span>
  <span aria-hidden="true" class={`${ checked ? 'translate-x-5' : 'translate-x-0' } pointer-events-none absolute left-0 inline-block h-5 w-5 transform rounded-full bg-white dark:bg-zinc-200 shadow ring-0 transition-transform duration-200 ease-in-out`}></span>
</button>