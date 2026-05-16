# Cloudflare Tunnel & Zero Trust Setup

This guide details how to securely expose the Smart Doorbell Dashboard (`http://localhost:3000`) to the public internet without opening any router ports, protected by an email-based One-Time Password (OTP) whitelist.

## Phase 1: Create the Tunnel (Cloudflare Dashboard)

1. Log into the [Cloudflare Zero Trust Dashboard](https://one.dash.cloudflare.com/).
2. On the left sidebar, navigate to **Networks > Tunnels**.
3. Click **Create a tunnel**.
4. Choose **Cloudflared** as the connector type and click Next.
5. Name the tunnel (e.g., `doorbell-pi`) and click **Save tunnel**.
6. Cloudflare will display an installation command for different operating systems.

## Phase 2: Install the Tunnel (Raspberry Pi)

SSH into your Raspberry Pi (`192.168.1.10`).

1. Select the **Debian** environment in the Cloudflare UI and choose the **64-bit** architecture (if you are running a 64-bit Pi OS, otherwise 32-bit/ARMv7).
2. Copy the provided command that looks like this:
   ```bash
   curl -L --output cloudflared.deb https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-arm64.deb
   sudo dpkg -i cloudflared.deb
   sudo cloudflared service install <YOUR_SECRET_TOKEN>
   ```
3. Run those commands on the Pi. Once installed, the daemon will automatically start and connect to Cloudflare.
4. Back in the Cloudflare Dashboard, the connector status should change to **Connected**. Click **Next**.

## Phase 3: Route the Traffic

1. In the **Public Hostnames** tab, configure the routing:
   * **Subdomain:** `doorbell`
   * **Domain:** `calebhabesh.com`
   * **Path:** *(leave blank)*
   * **Service Type:** `HTTP`
   * **URL:** `localhost:3000` (Since `cloudflared` is running on the same Pi as the dashboard)
2. Click **Save hostname**.

At this point, visiting `https://doorbell.example.com` will load your Next.js dashboard, but it is entirely public.

## Phase 4: Secure with Cloudflare Access (Zero Trust)

Now, we put the authentication wall in front of it.

1. In the Zero Trust left sidebar, go to **Access > Applications**.
2. Click **Add an application** and select **Self-hosted**.
3. **Application Configuration:**
   * **Application name:** `Smart Doorbell`
   * **Session Duration:** `1 Month` (so you don't have to constantly log in on your phone)
   * **Application domain:** `doorbell.example.com`
   * Click **Next**.
4. **Add a Policy:**
   * **Policy name:** `Allow Household Emails`
   * **Action:** `Allow`
   * **Include rules:** 
     * *Selector:* `Emails` (or `Emails ending in` for a whole domain)
     * *Value:* Type the allowed email addresses (e.g., `youremail@example.com`, `partner@example.com`).
   * Click **Next** and then **Add application**.

## Result

Whenever someone clicks the link in the `ntfy` push notification (`https://doorbell.example.com`), they will be intercepted by a Cloudflare login screen. Only users whose emails are in the policy can request a login code. Once they enter the OTP sent to their email, they gain access to the dashboard for 1 month.