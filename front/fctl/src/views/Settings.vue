<template>
    <v-container>
        <v-form validate-on="submit" @submit.prevent="submit">
            <v-autocomplete
                v-model="ssid"
                :items="items"
                label="SSID"
                :rules="[(v) => !!v || 'Enter SSID']"
            ></v-autocomplete>
            <v-text-field
                class="mt-2"
                v-model="password"
                label="Password"
                type="password"
                :rules="[(v) => !!v || 'Enter password']"
            ></v-text-field>
            <v-btn class="mt-2" color="primary" type="submit" block>Connect</v-btn>
        </v-form>
    </v-container>
</template>
<script setup>
import { ref } from 'vue'
import { mdiRefresh } from '@mdi/js'
const items = ['x1ongR']
const ssid = ref(null)
const password = ref(null)
async function submit(event) {
    const results = await event
    console.log(JSON.stringify(results, null, 2))
}
</script>
