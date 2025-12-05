import os
import ssl
import asyncio
import aiohttp
import certifi
from datetime import datetime as dt
from bs4 import BeautifulSoup
from core.utils.path_utils import resource_path
from core.logger import logger
from core.configs import Configs
from core.orders import OrderList

ORDER_PATH = "tmp/reports/"


class AuthOnCM:
    def __init__(self, base_tmp):
        self.session: aiohttp.ClientSession | None = None
        self.csrf_token: str | None = None
        self.configs = Configs()
        self.base_url = ""
        self.login_code_url = ""
        self.username = self.configs.username
        self.password = self.configs.password
        self.base_tmp = base_tmp
        self.reports_dir = os.path.join(base_tmp, "reports")
        os.makedirs(self.reports_dir, exist_ok=True)

    async def login(self):
        logger.info("Starting aiohttp session...")
        ssl_context = ssl.create_default_context(cafile=certifi.where())
        self.session = aiohttp.ClientSession(
            connector=aiohttp.TCPConnector(ssl=ssl_context),
            cookie_jar=aiohttp.CookieJar(unsafe=True),
            headers={
                "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64; aiohttp-client)"
            },
        )
        try:
            logger.info(f"Getting CSRF from https://lanx.cargamaquina.com.br/ ...")

            async with self.session.get(
                "https://lanx.cargamaquina.com.br/", allow_redirects=True
            ) as r:
                r.raise_for_status()
                final_url: str = str(r.url)
                logger.info(f"Redirecting to {final_url}...")
                self.base_url = final_url.split("/site")[0]
                self.login_code_url = final_url.split("/c/")[-1]
                html = await r.text()

            soup = BeautifulSoup(html, "html.parser")
            csrf_input = soup.find("input", {"name": "YII_CSRF_TOKEN"})

            if not csrf_input or "value" not in csrf_input.attrs:
                raise Exception("CSRF token not found.")

            self.csrf_token = csrf_input["value"]
            logger.info("CSRF token extracted.")

            login_payload = {
                "YII_CSRF_TOKEN": self.csrf_token,
                "LoginForm[username]": self.username,
                "LoginForm[password]": self.password,
                "LoginForm[codigoConexao]": f"{self.login_code_url}",
                "yt0": "Entrar",
            }

            logger.info(
                f"Sending login request to {self.base_url}/site/login/c/{self.login_code_url}..."
            )
            async with self.session.post(
                f"{self.base_url}/site/login/c/{self.login_code_url}",
                data=login_payload,
            ) as r:
                r.raise_for_status()

            logger.info("✅ Login successful!")
            return True

        except Exception as e:
            logger.exception(f"Login failed: {e}")
            await self.close()
            return False

    async def get_client(self) -> aiohttp.ClientSession:
        self.configs.load()
        if self.session and not self.session.closed:
            return self.session
        logger.warning("Session invalid or closed — re-authenticating...")
        ok = await self.login()
        if not ok:
            raise RuntimeError("Unable to create authenticated session.")

        return self.session

    async def close(self):
        if self.session and not self.session.closed:
            await self.session.close()
            logger.info("Session closed.")

    async def get_orders_by_code(self, code: str):
        now = dt.now().strftime("%d-%m-%Y")
        order_list: OrderList = OrderList()
        async with await self.get_client() as session:
            params = {
                "OrdemProducao[codigo]": f"{code}",
                "OrdemProducao[_nomeCliente]": "",
                "OrdemProducao[_nomeMaterial]": "",
                "OrdemProducao[status_op_id]": "Todos",
                "OrdemProducao[_etapasPlanejadas]": "",
                "OrdemProducao[forecast]": "0",
                "OrdemProducao[_inicioCriacao]": "",
                "OrdemProducao[_fimCriacao]": "",
                "OrdemProducao[_inicioEntrega]": "03/12/2025",
                "OrdemProducao[_fimEntrega]": "25/01/2026",
                "OrdemProducao[_limparFiltro]": "0",
                "pageSize": "20",
            }
            async with session.get(
                f"{self.base_url}/ordemProducao/exportarOrdens", params=params
            ) as r:
                r.raise_for_status()

                soup = BeautifulSoup(await r.text(), "html.parser")
                if "Nenhuma ordem de produção encontrada." in soup.text:
                    logger.info("No orders found.")
                    return

                table = soup.find("table")
                if not table:
                    logger.warning("No table found.")
                    return

                trs = table.find_all("tr")[1:]
                if not trs:
                    logger.warning("No rows found.")
                    return

                for tr in trs:
                    tds = tr.find_all("td")
                    if not tds:
                        continue

                    deliver_date = dt.fromisoformat(tds[1].text.strip())
                    code = int(tds[2].text.strip().split("-")[-1])
                    product = str(tds[4].text.strip())
                    description = str(tds[5].text.strip())
                    quantity = int(tds[6].text.strip())

                    order_list.create_order(
                        deliver_date=deliver_date,
                        code=code,
                        product=product,
                        description=description,
                        quantity=quantity,
                    )

                logger.info("✅ Orders fetched successfully!")

                file_path = os.path.join(self.reports_dir, f"{now}_orders.json")

                json_data = order_list.to_json()

                with open(file_path, "w", encoding="utf-8") as f:
                    f.write(json_data)

                logger.info(f"💾 JSON file saved: {file_path}")

                logger.info("✅ Orders fetched!")
                return order_list.to_dict()


async def main():
    session = AuthOnCM()
    ok = await session.login()
    if ok:
        await session.get_orders()
    await session.close()


if __name__ == "__main__":
    asyncio.run(main())
