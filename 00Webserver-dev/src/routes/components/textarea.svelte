<script>
  import { createEventDispatcher } from 'svelte';
  const dispatch = createEventDispatcher();
  let value;
  export let command = 'interaction';
  export let param = 'text';
  export let placeholder='Enter text';
  function sendData() {
      dispatch('update', { command:command, param:param, value:value });
      console.log('Text sent');
  }
</script>

<textarea class="textarea textarea-bordered textarea-lg" placeholder={placeholder} param={param} bind:value={value} on:input={sendData}></textarea>