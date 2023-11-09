<script>
  import { createEventDispatcher } from 'svelte';
  const dispatch = createEventDispatcher();
  
  let ssid;
  let password;

  function sendData() {
      dispatch('update', { command:'wifi', ssid: ssid, password: password});
      console.log('Card sent');
  }
</script>


<h3 class="font-bold text-lg">Connect to WiFi</h3>
<form w-full max-w-xs on:submit|preventDefault={sendData}>
  <input type="text" placeholder="SSID" bind:value={ssid} class="input input-bordered w-full max-w-xs" />
  <input type="text" placeholder="password" bind:value={password}  class="input input-bordered w-full max-w-xs" />
    <button class="btn" type="submit">Submit</button>
</form>