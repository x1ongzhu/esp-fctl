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
            <v-select
                v-model="ssid"
                :items="aps"
                item-title="ssid"
                item-value="ssid"
                label="SSID"
                :rules="[(v) => !!v || 'Select SSID']"
            >
            </v-select>
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
            <v-btn class="mt-2" color="primary" type="submit" block :disabled="scanning">{{
                scanning ? 'SCANNING' : 'Connect'
            }}</v-btn>
        </v-form>
    </v-container>
    <v-snackbar v-model="showSnackbar" :color="color">{{ msg }}</v-snackbar>
</template>
<script setup>
import { ref, onMounted } from 'vue'
import axios from 'axios'
import { computed } from 'vue'
import { mdiRefresh } from '@mdi/js'
const items = ref([{ ssid: 'sdfasdfkasj', rssi: -50 }])
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
const scanning = ref(false)
const name = ref('')

async function getName() {
    try {
        const res = await axios.get('/name')
        name.value = res.data.name
    } catch (error) {}
}

async function scan() {
    scanning.value = true
    try {
        const res = await axios.get('/wifi/scan')
        items.value = res.data
        scanning.value = false
    } catch (error) {
        scanning.value = false
    }
}

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
onMounted(async () => {
    await getName()
    await scan()
})
</script>
