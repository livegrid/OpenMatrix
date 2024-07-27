<script>
  export let modeId, active, title, icon, disabled;
  import { state } from "@/store";

  $: activeColor = active ? 'text-emerald-600' : 'text-gray-500 dark:text-zinc-400';
</script>

<button on:click disabled={disabled} class={`flex items-center justify-between ${ active === true ? 'bg-gray-100 dark:bg-emerald-900/20 border border-gray-200 dark:border-zinc-900' : 'border border-transparent hover:bg-gray-100 disabled:hover:bg-transparent disabled:dark:hover:bg-transparent dark:hover:bg-zinc-900/60' } ${activeColor} hover:text-emerald-600 group disabled:opacity-50 flex gap-x-3 rounded-md p-2 py-2.5 text-sm leading-6 font-semibold`}>
  <span class="flex flex-row flex-grow items-center space-x-3">
    {#if icon === 'home'}
      <svg class={`h-5 w-5 shrink-0 ${activeColor} group-hover:text-emerald-600`} xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m3 9 9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/><polyline points="9 22 9 12 15 12 15 22"/></svg>
    {:else if icon === 'fish'}
      <svg class={`h-5 w-5 shrink-0 ${activeColor} group-hover:text-emerald-600`} xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M2 16s9-15 20-4C11 23 2 8 2 8"/></svg>
    {:else if icon === 'sparkles'}
      <svg class={`h-5 w-5 shrink-0 ${activeColor} group-hover:text-emerald-600`} xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M9.937 15.5A2 2 0 0 0 8.5 14.063l-6.135-1.582a.5.5 0 0 1 0-.962L8.5 9.936A2 2 0 0 0 9.937 8.5l1.582-6.135a.5.5 0 0 1 .963 0L14.063 8.5A2 2 0 0 0 15.5 9.937l6.135 1.581a.5.5 0 0 1 0 .964L15.5 14.063a2 2 0 0 0-1.437 1.437l-1.582 6.135a.5.5 0 0 1-.963 0z"/><path d="M20 3v4"/><path d="M22 5h-4"/><path d="M4 17v2"/><path d="M5 18H3"/></svg>
    {:else if icon === 'image'}
      <svg class={`h-5 w-5 shrink-0 ${activeColor} group-hover:text-emerald-600`} xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect width="18" height="18" x="3" y="3" rx="2" ry="2"/><circle cx="9" cy="9" r="2"/><path d="m21 15-3.086-3.086a2 2 0 0 0-2.828 0L6 21"/></svg>
    {:else if icon === 'text'}
      <svg class={`h-5 w-5 shrink-0 ${activeColor} group-hover:text-emerald-600`} xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="4 7 4 4 20 4 20 7"/><line x1="9" x2="15" y1="20" y2="20"/><line x1="12" x2="12" y1="4" y2="20"/></svg>
    {:else if icon === 'settings'}
      <svg class={`h-5 w-5 shrink-0 ${activeColor} group-hover:text-emerald-600`} xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M20 7h-9"/><path d="M14 17H5"/><circle cx="17" cy="17" r="3"/><circle cx="7" cy="7" r="3"/></svg>
    {/if}
    <span>
      {title}
    </span>
  </span>
  {#if $state.mode === modeId}
    <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="currentColor" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-4 h-4 text-emerald-600 me-1"><polygon points="6 3 20 12 6 21 6 3"/></svg>
  {/if}
</button>
