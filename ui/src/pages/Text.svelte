
<script>
  import Button from "@/components/Button.svelte";
  import { onDestroy, onMount } from "svelte";
  import { state, updateText } from "@/store";
  import { get } from 'svelte/store';
  export let color = '#ffffff';
  export let onColorChange;
  let showColorPicker = false;

  let payload = null;
  let size = 0;
  let loading = false;

  let sizes = [
    {
      id: 0,
      name: 'S'
    },
    {
      id: 1,
      name: 'M'
    },
    {
      id: 2,
      name: 'L'
    }
  ];

  const update = async () => {
    loading = true;

    try {
      await updateText({
        payload,
        size
      });
    } catch (err) {
      console.log(err);
    }
    loading = false;
  };

  onMount(() => {
    payload = get(state)?.text?.payload;
    size = get(state)?.text?.size;
  });

  const unsubscribe = state.subscribe((value) => {
    // upon fresh load
    if (payload == null) {
      payload = value?.text?.payload;
      size = value?.text?.size;
    }
  });

  onDestroy(() => {
    unsubscribe();
  });

  function toggleColorPicker() {
    console.log("Toggle color picker clicked");
    showColorPicker = !showColorPicker;
  }

  function handleColorChange(event) {
    const newColor = event.target.value;
    onColorChange(newColor);
  }
</script>

<!-- Your content -->
<div>
  <div class="flex flex-wrap items-center gap-6 sm:flex-nowrap sm:justify-between">
    <h1 class="text-base text-lg font-semibold leading-7 text-gray-900 dark:text-zinc-300">Text</h1>
  </div>
</div>

<div class="mt-8 relative">
  <form>
    <div class="w-full mb-4 border border-gray-200 rounded-md bg-white/50 dark:bg-zinc-950/50 dark:border-zinc-900">
      <div class="px-4 py-2 bg-transparent rounded-t-lg">
        <label for="text" class="sr-only">Your text</label>
        <textarea id="text" rows="4" bind:value={payload} class="w-full px-0 text-sm text-gray-900 bg-transparent border-0 focus:ring-0 dark:text-white dark:placeholder-gray-400" placeholder="Write a message..." required ></textarea>
      </div>
      <div class="flex items-center justify-between px-4 py-3 border-t dark:border-zinc-900">
        {#if payload !== $state?.text?.payload || size !== $state?.text?.size}
          <Button
            size='l'
            className='px-4' on:click={update} 
            loading={loading}
          >
            Save
          </Button>
        {/if}
        <div class="flex space-x-2 rtl:space-x-reverse order-first items-center">
          {#each sizes as s (s.id)}
            <Button type={'button'} on:click={() => size = s.id} className={s.id === size ? 'w-[40px] justify-center text-center !border-emerald-600 !dark:border-emerald-600 !hover:border-emerald-600 !dark:hover:border-emerald-600' : 'w-[40px] justify-center'}>
              <span class={s.id === size ? 'text-emerald-600 dark:text-emerald-600' : ''}>
                {s.name}
              </span>
            </Button>
          {/each}
          <button
            type="button"
            class="w-3 h-3 aspect-square rounded-md border border-gray-300 mr-2"
            style="background-color: {color};"
            on:click={() => showColorPicker = !showColorPicker}
          ></button>
        </div>
      </div>
    </div>
  </form>
  {#if showColorPicker}
    <div class="absolute right-0 top-full mt-2 z-50">
      <input
        type="color"
        value={color}
        on:change={handleColorChange}
      />
    </div>
  {/if}
</div>