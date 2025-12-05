import json
from dataclasses import dataclass
from datetime import datetime
from typing import List, Dict


@dataclass
class Order:
    deliver_date: datetime
    code: int
    product: str
    description: str
    quantity: int

    def to_dict(self):
        return {
            "deliver_date": self.deliver_date.strftime("%Y-%m-%d %H:%M:%S"),
            "code": self.code,
            "product": self.product,
            "description": self.description,
            "quantity": self.quantity,
        }


class OrderList:
    def __init__(self):
        self.orders: List[Order] = []

    def create_order(
        self,
        deliver_date: datetime,
        code: int,
        product: str,
        description: str,
        quantity: int,
    ) -> Order:
        order = Order(
            deliver_date=deliver_date,
            code=code,
            product=product,
            description=description,
            quantity=quantity,
        )
        self.orders.append(order)
        return order

    def to_dict(self) -> Dict[str, dict]:
        return {str(o.code): o.to_dict() for o in self.orders}

    def to_json(self):
        return json.dumps(self.to_dict(), ensure_ascii=False, indent=4)


class OrderManager:
    def __init__(self):
        self.orders = ""
