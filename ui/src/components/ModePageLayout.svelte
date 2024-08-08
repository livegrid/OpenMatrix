<script>
    // @ts-nocheck
    import PlayButton from "@/components/PlayButton.svelte";
    import { onDestroy } from "svelte";
    import { state, selectMode } from "@/store";
    export let name, id;

    let loading = null, target = null;

    const unsubscribe = state.subscribe((value) => {
        if (value?.mode === target) {
            target = value?.mode;
            loading = false;
        }
    });
  
    onDestroy(() => {
        unsubscribe();
    });
</script>

<div>
    <div class="flex flex-wrap items-center gap-6 sm:flex-nowrap justify-between">
        <h1 class="text-base text-lg font-semibold leading-7 text-gray-900 dark:text-zinc-300">
            {name}
        </h1>
        <PlayButton
            initializing={Object.keys($state).length === 0}
            loading={loading}
            active={$state.mode === id}
            on:click={() => {
                loading = true;
                target = id;
                selectMode(id);
            }}  
        />
    </div>
</div>
<div class="mt-8">
    <slot />
</div>