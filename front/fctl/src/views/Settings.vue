<template>
    <v-container>
        <v-form validate-on="submit" @submit.prevent="saveName">
            <v-text-field
                class="mt-2"
                v-model="name"
                label="NAME"
                :rules="[(v) => !!v || 'Enter Name']"
            ></v-text-field>
            <v-btn class="mt-2" color="primary" type="submit" block>SAVE NAME</v-btn>
        </v-form>

        <v-form validate-on="submit" @submit.prevent="connect" class="mt-16">
            <v-autocomplete
                v-model="ssid"
                :items="aps"
                label="SSID"
                :rules="[(v) => !!v || 'Enter SSID']"
            ></v-autocomplete>
            <v-text-field
                class="mt-2"
                v-model="password"
                label="PASSWORD"
                type="PASSWORD"
                :rules="[
                    (v) => !!v || 'Enter Password',
                    (v) => v.length >= 8 || 'Password must be at least 8 characters'
                ]"
            ></v-text-field>
            <v-btn class="mt-2" color="primary" type="submit" block>Connect</v-btn>
        </v-form>
    </v-container>
    <v-snackbar v-model="showSnackbar" :color="color">{{ msg }}</v-snackbar>
</template>
<script setup>
import { ref } from 'vue'
import axios from 'axios'
import { computed } from 'vue'
const items = ref([])
const aps = computed(() => {
    return items.value
        .map((item) => item.ssid)
        .filter((value, index, array) => {
            return array.indexOf(value) === index
        })
})
const ssid = ref(null)
const password = ref(null)
const showSnackbar = ref(false)
const msg = ref('')
const color = ref('success')
axios.get('/wifi/scan').then((res) => {
    items.value = res.data
})
async function connect(event) {
    const { valid } = await event
    if (!valid) return
    try {
        const res = await axios.post('/wifi/sta', { ssid: ssid.value, password: password.value })
        color.value = 'success'
        msg.value = 'Success'
        showSnackbar.value = true
    } catch (e) {
        color.value = 'error'
        msg.value = 'Faild'
        showSnackbar.value = true
    }
}

const name = ref('')
axios.get('/name').then((res) => {
    name.value = res.data.name
})
async function saveName(event) {
    const { valid } = await event
    if (!valid) return
    try {
        const res = await axios.put('/name', { name: name.value })
        color.value = 'success'
        msg.value = 'Success'
        showSnackbar.value = true
    } catch (e) {
        color.value = 'error'
        msg.value = 'Faild'
        showSnackbar.value = true
    }
}
</script>
