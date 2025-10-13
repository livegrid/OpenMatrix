<script>
  import TextButton from "./TextButton.svelte";
  import EffectSettingsPopup from "./EffectSettingsPopup.svelte";
  export let image, name, selected, loading;
  let color = '#ffffff'; // Used in EffectSettingsPopup
  let speed = 50; // Used in EffectSettingsPopup
  let scale = 50; // Used in EffectSettingsPopup
  export let onColorChange;
  export let onSpeedChange;
  export let onScaleChange;
  export let onParticleCountChange;
  export let onComplexityChange;
  
  function handleColorChange(newColor) {
    onColorChange(newColor);
  }

  function handleSpeedChange(newSpeed) {
    onSpeedChange(newSpeed);
  }

  function handleScaleChange(newScale) {
    onScaleChange(newScale);
  }

  function handleParticleCountChange(newParticleCount) {
    onParticleCountChange(newParticleCount);
  }

  function handleComplexityChange(newComplexity) {
    onComplexityChange(newComplexity);  
  }
</script>

<div class="relative">
  <div class="relative flex flex-col overflow-hidden rounded-md border border-gray-200 dark:border-zinc-900 bg-white/80 dark:bg-black/30 px-4 py-5 sm:px-6 sm:py-6 hover:shadow-2xl transition-shadow duration-300 ease-in-out hover:shadow-emerald-500/20 dark:hover:shadow-emerald-900/50">
    <div class="flex items-center gap-x-4 mb-4">
      <div class="flex-shrink-0">
        <svelte:component this={image} />
      </div>
      <div class="min-w-0 flex-1">
        <h5 class="truncate text-lg font-medium text-gray-800 dark:text-gray-300">{name}</h5>
      </div>
    </div>
    <div class="flex justify-end items-center mt-auto">
      <TextButton on:click loading={loading} selected={selected} text="Select"/>
    </div>
  </div>
  
</div>